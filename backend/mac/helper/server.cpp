#include "server.h"

#include <assert.h>
#include <sstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "firewallcontroller.h"
#include "ipc/helper_security.h"
#include "logger.h"
#include "process_command.h"
#include "utils.h"

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

    LOG("Client connected");
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
            LOG("Client disconnected with XPC_ERROR_CONNECTION_INVALID");
        } else if (event == XPC_ERROR_TERMINATION_IMMINENT) {
            // Handle per-connection termination cleanup
            xpc_connection_cancel(peer);
            LOG("Client disconnected with XPC_ERROR_TERMINATION_IMMINENT");
        } else {
            LOG("Client disconnected with an unknown error");
        }

    } else if (type == XPC_TYPE_DICTIONARY) {
        // check the validity of the process that sent the command
        // right here, because event must be of type XPC_TYPE_DICTIONARY
        if (!HelperSecurity::isValidXpcConnection(event)) {
            xpc_connection_cancel(peer);
            LOG("Client disconnected with failed validity test");
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

        CMD_ANSWER cmdAnswer = processCommand(cmdId, data);

        // send answer
        std::stringstream stream;
        boost::archive::text_oarchive oa(stream, boost::archive::no_header);
        oa << cmdAnswer;
        std::string str = stream.str();
        xpc_object_t message = xpc_dictionary_create_reply(event);
        xpc_dictionary_set_data(message, "data", str.data(), str.length());
        xpc_connection_send_message(peer, message);
        xpc_release(event);
        xpc_release(message);
    } else {
        LOG("Client sent an unknown message");
    }
}
void Server::run()
{
    if (Utils::isAppUninstalled()) {
        Utils::deleteSelf();
        return;
    }

    Utils::createWindscribeUserAndGroup();

    system("mkdir -p /var/run/windscribe && chown :windscribe /var/run/windscribe && chmod 777 /var/run/windscribe");
    system("mkdir -p /etc/windscribe");

    xpc_connection_t listener = xpc_connection_create_mach_service("com.windscribe.helper.macos", NULL, XPC_CONNECTION_MACH_SERVICE_LISTENER);
    if (!listener) {
        LOG("xpc_connection_create_mach_service failed");
        return;
    }

    xpc_connection_set_event_handler(listener, ^(xpc_object_t event) {
        // New connections arrive here. You may safely cast to xpc_connection_t
        xpc_event_handler((xpc_connection_t)event);
    });

    // Cause the FirewallController to be constructed here, so that on-boot rules are processed, even if the Windscribe app/service does not start.
    FirewallController::instance();

    xpc_connection_resume(listener);

    // this method does not return
    dispatch_main();
}
