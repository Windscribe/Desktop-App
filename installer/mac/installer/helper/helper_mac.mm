#include "helper_mac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
#include <spdlog/spdlog.h>

#include "../../../../backend/posix_common/helper_commands_serialize.h"
#include "../string_utils.h"

Helper_mac::Helper_mac() : connection_(nullptr)
{
}

Helper_mac::~Helper_mac()
{
    stop();
}

bool Helper_mac::connect()
{
    std::lock_guard locker(mutex_);

    connection_ = xpc_connection_create_mach_service("com.windscribe.helper.macos", NULL, 0);
    if (!connection_) {
        spdlog::error("xpc_connection_create_mach_service failed");
        return false;
    }

    xpc_connection_set_event_handler(connection_, ^(xpc_object_t event) {
        xpc_type_t type = xpc_get_type(event);
        if (type == XPC_TYPE_ERROR) {
            if (event == XPC_ERROR_CONNECTION_INVALID) {
                // The client process on the other end of the connection has either
                // crashed or cancelled the connection. After receiving this error,
                // the connection is in an invalid state, and you do not need to
                // call xpc_connection_cancel(). Just tear down any associated state here.
                spdlog::info("Client disconnected with XPC_ERROR_CONNECTION_INVALID");
            } else if (event == XPC_ERROR_TERMINATION_IMMINENT) {
                // Handle per-connection termination cleanup
                spdlog::info("Client disconnected with XPC_ERROR_TERMINATION_IMMINENT");
            } else {
                spdlog::warn("Client disconnected with an uknown error");
            }
        } else {
          spdlog::error("xpc_connection_set_event_handler should not be here");
          assert(false);
        }
    });

    xpc_connection_resume(connection_);

    // Send the test command HELPER_CMD_HELPER_VERSION to make sure the helper is connected
    CMD_ANSWER answerCmd = sendCmdToHelper(HELPER_CMD_HELPER_VERSION, std::string());
    if (!answerCmd.body.empty()) {
        NSString *helperVersion = [NSString stringWithCString:answerCmd.body.c_str() encoding:[NSString defaultCStringEncoding]];
        spdlog::info("Helper connected, version: {}", toStdString(helperVersion));
        return true;
    } else {
        spdlog::error("Failed connect to helper");
        return false;
    }
}

void Helper_mac::stop()
{
    std::lock_guard locker(mutex_);

    if (connection_) {
        xpc_connection_cancel(connection_);
        connection_ = nullptr;
    }
}

bool Helper_mac::setPaths(const std::wstring &archivePath, const std::wstring &installPath)
{
    std::lock_guard locker(mutex_);
    CMD_INSTALLER_FILES_SET_PATH cmd;
    cmd.archivePath = archivePath;
    cmd.installPath = installPath;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;
    CMD_ANSWER answerCmd = sendCmdToHelper(HELPER_CMD_INSTALLER_SET_PATH, stream.str());
    return answerCmd.executed == 1;
}

bool Helper_mac::executeFilesStep()
{
    std::lock_guard locker(mutex_);
    CMD_ANSWER answerCmd = sendCmdToHelper(HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE, std::string());
    return answerCmd.executed == 1;
}

bool Helper_mac::deleteOldHelper()
{
    std::lock_guard locker(mutex_);
    return runCommand(HELPER_CMD_DELETE_OLD_HELPER, {});
}

bool Helper_mac::removeOldInstall(const std::string &path)
{
    std::lock_guard locker(mutex_);
    CMD_INSTALLER_REMOVE_OLD_INSTALL cmd;
    cmd.path = path;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_INSTALLER_REMOVE_OLD_INSTALL, stream.str());
}

bool Helper_mac::killWindscribeProcess()
{
    std::lock_guard locker(mutex_);
    CMD_TASK_KILL cmd;
    cmd.target = kTargetWindscribe;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_TASK_KILL, stream.str());
}

bool Helper_mac::createCliSymlink()
{
    std::lock_guard locker(mutex_);
    CMD_INSTALLER_CREATE_CLI_SYMLINK cmd;
    cmd.uid = getuid();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_INSTALLER_CREATE_CLI_SYMLINK, stream.str());
}

CMD_ANSWER Helper_mac::sendCmdToHelper(int cmdId, const std::string &data)
{
    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(message, "cmdId", cmdId);
    xpc_dictionary_set_data(message, "data", data.c_str(), data.size());
    xpc_object_t answer = xpc_connection_send_message_with_reply_sync(connection_, message);

    xpc_type_t type = xpc_get_type(answer);
    if (type == XPC_TYPE_ERROR) {
        spdlog::info("xpc_connection_send_message_with_reply_sync return XPC_TYPE_ERROR");
        return CMD_ANSWER();
    } else if (type == XPC_TYPE_DICTIONARY) {
        size_t length;
        const void *buf = xpc_dictionary_get_data(answer, "data", &length);
        if (buf && length > 0) {
            CMD_ANSWER cmdAnswer;
            std::string str((char *)buf, length);
            std::istringstream stream(str);
            boost::archive::text_iarchive ia(stream, boost::archive::no_header);
            ia >> cmdAnswer;
            return cmdAnswer;
        } else {
            return CMD_ANSWER();
        }
    } else {
      spdlog::warn("xpc_connection_send_message_with_reply_sync return an unknown message");
      return CMD_ANSWER();
    }
}

bool Helper_mac::runCommand(int cmdId, const std::string &data)
{
    CMD_ANSWER answerCmd = sendCmdToHelper(cmdId, data);
    if (!answerCmd.executed) {
        return false;
    }
    return true;
}
