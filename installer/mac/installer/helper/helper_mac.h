#pragma once

#include <string>
#include "../../../../backend/posix_common/helper_commands.h"

class Helper_mac
{
public:
    Helper_mac();
    ~Helper_mac();

    bool connect();
    void stop();

    bool setPaths(const std::wstring &archivePath, const std::wstring &installPath, uid_t userId, gid_t groupId);
    int executeFilesStep();

    bool killProcess(pid_t pid);
    bool killWindscribeProcess();

    bool removeOldInstall(const std::string &path);
    bool deleteOldHelper();

    bool createCliSymlinkDir();

private:
    bool sendCmdToHelper(int cmdId, const std::string &data);
    bool readAnswer(CMD_ANSWER &outAnswer);
    bool runCommand(int cmdId, const std::string &data, CMD_ANSWER &answer);
    bool sendAll(int s, void *buf, int len);
    bool recvAll(int s, void *buf, int len);

    int sock_;
};
