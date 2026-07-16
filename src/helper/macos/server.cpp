#include "server.h"

#include <assert.h>
#include <filesystem>
#include <grp.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include <spdlog/spdlog.h>
#include "../common/io_posix.h"
#include "firewallcontroller.h"
#include "ipc/helper_security.h"
#include "process_command.h"
#include "utils.h"

namespace {
// Scope-local umask override. Restores the previous umask in the destructor so an exception during the guarded
// operation can't leave the process-wide umask in an unexpected state.
struct UmaskGuard {
    explicit UmaskGuard(mode_t mask) : prev_(umask(mask)) {}
    ~UmaskGuard() { umask(prev_); }
    mode_t prev_;
};
} // namespace

Server::Server() : listener_(nullptr)
{
}

Server::~Server()
{
    if (listener_)
        xpc_connection_cancel(listener_);
}

void Server::xpc_event_handler(xpc_connection_t peer)
{
    // By defaults, new connections will target the default dispatch concurrent queue.
    xpc_connection_set_event_handler(peer, ^(xpc_object_t event) {
        peer_event_handler(peer, event);
    });

    // This will tell the connection to begin listening for events. If you
    // have some other initialization that must be done asynchronously, then
    // you can defer this call until after that initialization is done.
    xpc_connection_resume(peer);

    spdlog::debug("Client connected");
}

void Server::peer_event_handler(xpc_connection_t peer, xpc_object_t event)
{
    xpc_type_t type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        if (event == XPC_ERROR_CONNECTION_INVALID) {
            // The client process on the other end of the connection has either
            // crashed or cancelled the connection. After receiving this error,
            // the connection is in an invalid state, and you do not need to
            // call xpc_connection_cancel(). Just tear down any associated state here.
            spdlog::debug("Client disconnected with XPC_ERROR_CONNECTION_INVALID");
        } else if (event == XPC_ERROR_TERMINATION_IMMINENT) {
            // Handle per-connection termination cleanup
            xpc_connection_cancel(peer);
            spdlog::debug("Client disconnected with XPC_ERROR_TERMINATION_IMMINENT");
        } else {
            spdlog::error("Client disconnected with an unknown error");
        }

    } else if (type == XPC_TYPE_DICTIONARY) {
        // check the validity of the process that sent the command
        // right here, because event must be of type XPC_TYPE_DICTIONARY
        if (!HelperSecurity::isValidXpcConnection(event)) {
            xpc_connection_cancel(peer);
            spdlog::error("Client disconnected with failed validity test");
            return;
        }

        // handle the message
        xpc_retain(event);
        int64_t cmdId = xpc_dictionary_get_int64(event, "cmdId");
        std::string data;

        size_t length;
        const void *buf = xpc_dictionary_get_data(event, "data", &length);
        if (buf && length > 0) {
            data = std::string((const char *)buf, length);
        }

        // Pass the connection's kernel-verified peer uid down to the handler so per-user resources
        // (executeOpenVPN's management socket) can be locked to the caller without trusting anything
        // in the payload. It's passed as a call argument rather than stored in a shared global: peer
        // events dispatch on the default concurrent queue, so a static would race across concurrent
        // commands from different clients.
        std::string answer;
        try {
            answer = processCommand((HelperCommand)cmdId, data, xpc_connection_get_euid(peer));
        } catch (const std::exception &ex) {
            spdlog::error("Helper IPC: processCommand({}) exception: {}", (int)cmdId, ex.what());
            answer.clear();
        }
        HelperSecurity::clearCurrentCallerSecCode();

        xpc_object_t message = xpc_dictionary_create_reply(event);
        if (message) {
            xpc_dictionary_set_data(message, "data", answer.data(), answer.length());
            xpc_connection_send_message(peer, message);
            xpc_release(message);
        }
        xpc_release(event);
    } else {
        spdlog::error("Client sent an unknown message");
    }
}
void Server::run()
{
    if (Utils::isAppUninstalled()) {
        Utils::deleteSelf();
        return;
    }

    Utils::createAppUserAndGroup();

    std::error_code ec;
    {
        UmaskGuard guard(022);
        std::filesystem::create_directories(WS_POSIX_CONFIG_DIR, ec);
    }
    if (ec) {
        spdlog::error("Failed to create " WS_POSIX_CONFIG_DIR ": {}", ec.message());
    }
    // Enforce ownership before mode so that, if the dir pre-existed with a wrong owner, the chmod
    // doesn't first hand owner-write to that wrong owner during the window between calls.
    if (::lchown(WS_POSIX_CONFIG_DIR, 0, 0) != 0) {
        spdlog::error("Failed to chown " WS_POSIX_CONFIG_DIR ": {}", IO::strerror(errno));
    }
    std::filesystem::permissions(WS_POSIX_CONFIG_DIR,
        std::filesystem::perms::owner_all
            | std::filesystem::perms::group_read | std::filesystem::perms::group_exec
            | std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
        ec);
    if (ec) {
        spdlog::error("Failed to set permissions on " WS_POSIX_CONFIG_DIR ": {}", ec.message());
    }

    xpc_connection_t listener = xpc_connection_create_mach_service(WS_MAC_HELPER_BUNDLE_ID, NULL, XPC_CONNECTION_MACH_SERVICE_LISTENER);
    if (!listener) {
        spdlog::error("xpc_connection_create_mach_service failed");
        return;
    }

    xpc_connection_set_event_handler(listener, ^(xpc_object_t event) {
        // New connections arrive here. You may safely cast to xpc_connection_t
        xpc_event_handler((xpc_connection_t)event);
    });

    // Cause the FirewallController to be constructed here, so that on-boot rules are processed, even if the app/service does not start.
    FirewallController::instance();

    xpc_connection_resume(listener);

    // this method does not return
    dispatch_main();
}
