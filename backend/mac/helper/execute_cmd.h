#pragma once

#include <stdio.h>
#include <string>
#include <list>
#include <mutex>

class ExecuteCmd
{
public:
    static ExecuteCmd &instance()
    {
        static ExecuteCmd i;
        return i;
    }

    unsigned long execute(const std::string &cmd, const std::string &cwd = "", bool deleteOnFinish = false);
    void getStatus(unsigned long cmdId, bool &bFinished, std::string &log);
    void clearCmds();

private:
    ExecuteCmd();

    unsigned long curCmdId_;

    static void runCmd(unsigned long cmdId, std::string cmd, bool deleteOnFinish);
    void cmdFinished(unsigned long cmdId, bool bSuccess, std::string log, bool del);
    bool isCmdExist(unsigned long cmdId);

    struct CmdDescr
    {
        unsigned long cmdId;
        std::string log;
        bool bFinished;
        bool bSuccess;
    };

    std::list<CmdDescr *> executingCmds_;
    std::mutex mutex_;
};
