#include "helper_mac.h"
#include "utils/logger.h"
#include <QStandardPaths>
#include "utils/ws_assert.h"
#include <QCoreApplication>
#include "installhelper_mac.h"
#include "../../../../backend/posix_common/helper_commands_serialize.h"

Helper_mac::Helper_mac(QObject *parent) : Helper_posix(parent), connection_(nullptr)
{

}

Helper_mac::~Helper_mac()
{
    curState_ = STATE_INIT;
    if (connection_) {
        xpc_connection_cancel(connection_);
    }
}

void Helper_mac::startInstallHelper()
{
    bool isUserCanceled;
    if (InstallHelper_mac::installHelper(isUserCanceled))
    {
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
                if (curState_ != STATE_INIT)
                    emit lostConnectionToHelper();
            } else {
                // should not be here
                WS_ASSERT(false);
            }
        });

        xpc_connection_resume(connection_);
        curState_ = STATE_CONNECTED;
    }
    else
    {
        if (isUserCanceled)
        {
            curState_ = STATE_USER_CANCELED;
        }
        else
        {
            curState_ = STATE_INSTALL_FAILED;
        }
    }
}

bool Helper_mac::reinstallHelper()
{
    InstallHelper_mac::uninstallHelper();
    return true;
}


QString Helper_mac::getHelperVersion()
{
    QMutexLocker locker(&mutex_);
    CMD_ANSWER answer;
    if (runCommand(HELPER_CMD_HELPER_VERSION, std::string(), answer))
        return QString::fromStdString(answer.body);
    else
        return "";
}

bool Helper_mac::setMacAddress(const QString &interface, const QString &macAddress)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_MAC_ADDRESS cmd;
    CMD_ANSWER answer;
    cmd.interface = interface.toStdString();
    cmd.macAddress = macAddress.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_MAC_ADDRESS, stream.str(), answer);

}

bool Helper_mac::enableMacSpoofingOnBoot(bool bEnabled, const QString &interface, const QString &macAddress)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_MAC_SPOOFING_ON_BOOT cmd;
    CMD_ANSWER answer;
    cmd.enabled = bEnabled;
    cmd.interface = interface.toStdString();
    cmd.macAddress = macAddress.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_MAC_SPOOFING_ON_BOOT, stream.str(), answer);
}

bool Helper_mac::setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &entry)
{
    QMutexLocker locker(&mutex_);

    CMD_APPLY_CUSTOM_DNS cmd;
    cmd.ipAddress = ipAddress.toStdString();
    cmd.networkService = entry.toStdString();

    if (curState_ != STATE_CONNECTED)
        return false;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (runCommand(HELPER_CMD_APPLY_CUSTOM_DNS, stream.str(), answer)) {
        return answer.executed != 0;
    } else {
        return false;
    }
}

bool Helper_mac::setIpv6Enabled(bool bEnabled)
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    CMD_SET_IPV6_ENABLED cmd;
    cmd.enabled = bEnabled;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_IPV6_ENABLED, stream.str(), answer);
}

void Helper_mac::doDisconnectAndReconnect()
{
    // nothing todo
}

bool Helper_mac::runCommand(int cmdId, const std::string &data, CMD_ANSWER &answer)
{
    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(message, "cmdId", cmdId);
    xpc_dictionary_set_data(message, "data", data.c_str(), data.size());
    xpc_object_t answer_object = xpc_connection_send_message_with_reply_sync(connection_, message);

    xpc_type_t type = xpc_get_type(answer_object);
    if (type == XPC_TYPE_ERROR) {
        return false;
    } else if (type == XPC_TYPE_DICTIONARY) {
        size_t length;
        const void *buf = xpc_dictionary_get_data(answer_object, "data", &length);
        if (buf && length > 0) {
            std::string str((char *)buf, length);
            std::istringstream stream(str);
            boost::archive::text_iarchive ia(stream, boost::archive::no_header);
            ia >> answer;
            return true;
        } else {
            return false;
        }
    }

    return false;
}
