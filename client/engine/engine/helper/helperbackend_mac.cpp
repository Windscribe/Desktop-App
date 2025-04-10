#include "helperbackend_mac.h"
#include <QMutexLocker>
#include "installhelper_mac.h"

HelperBackend_mac::HelperBackend_mac(QObject *parent, spdlog::logger *logger) :
    IHelperBackend(parent), connection_(nullptr), logger_(logger)
{

}

HelperBackend_mac::~HelperBackend_mac()
{
    curState_ = State::kInit;
    if (connection_) {
        xpc_connection_cancel(connection_);
    }
}

void HelperBackend_mac::startInstallHelper()
{
    bool isUserCanceled;
    if (InstallHelper_mac::installHelper(false, isUserCanceled, logger_)) {
        // start XPC connection
        connection_ = xpc_connection_create_mach_service("com.windscribe.helper.macos", NULL, 0);
        if (!connection_)
            return;

        xpc_connection_set_event_handler(connection_, ^(xpc_object_t event) {
            xpc_type_t type = xpc_get_type(event);
            if (type == XPC_TYPE_ERROR) {
                if (event == XPC_ERROR_CONNECTION_INVALID) {
                    // The client process on the other end of the connection has either
                    // crashed or cancelled the connection. After receiving this error,
                    // the connection is in an invalid state, and you do not need to
                    // call xpc_connection_cancel(). Just tear down any associated state here.
                } else if (event == XPC_ERROR_TERMINATION_IMMINENT) {
                    // Handle per-connection termination cleanup

                } else {

                }
                if (curState_ != State::kInit)
                    emit lostConnectionToHelper();
            } else {
                // should not be here
                logger_->error("HelperBackend_mac::startInstallHelper fatal error, should not be here");
            }
        });

        xpc_connection_resume(connection_);
        curState_ = State::kConnected;
    } else {
        if (isUserCanceled){
            curState_ = State::kUserCanceled;
        } else {
            curState_ = State::kInstallFailed;
        }
    }
}

HelperBackend_mac::State HelperBackend_mac::currentState() const
{
    return curState_;
}

bool HelperBackend_mac::reinstallHelper()
{
    InstallHelper_mac::uninstallHelper(logger_);
    return true;
}

std::string HelperBackend_mac::sendCmd(int cmdId, const std::string &data)
{
    QMutexLocker locker(&mutex_);

    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(message, "cmdId", cmdId);
    xpc_dictionary_set_data(message, "data", data.c_str(), data.size());
    xpc_object_t answer_object = xpc_connection_send_message_with_reply_sync(connection_, message);

    auto exitGuard = qScopeGuard([&]() {
        xpc_release(message);
        xpc_release(answer_object);
    });

    xpc_type_t type = xpc_get_type(answer_object);
    if (type == XPC_TYPE_ERROR) {
        emit lostConnectionToHelper();
        return std::string();
    } else if (type == XPC_TYPE_DICTIONARY) {
        size_t length;
        const void *buf = xpc_dictionary_get_data(answer_object, "data", &length);
        if (buf && length > 0) {
            return std::string((char *)buf, length);
        }
    }
    return std::string();
}
