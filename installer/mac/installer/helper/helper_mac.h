#pragma once

#include <string>
#include <xpc/xpc.h>
#include <mutex>
#include "../../../../backend/posix_common/helper_commands.h"

class Helper_mac
{
public:
    Helper_mac();
    ~Helper_mac();

    bool connect();
    void stop();

    bool setPaths(const std::wstring &archivePath, const std::wstring &installPath);
    bool executeFilesStep();

    bool killWindscribeProcess();

    bool removeOldInstall(const std::string &path);
    bool deleteOldHelper();

    bool createCliSymlink();

private:
    CMD_ANSWER sendCmdToHelper(int cmdId, const std::string &data);
    bool runCommand(int cmdId, const std::string &data);

    std::mutex mutex_;
    xpc_connection_t connection_;
};
