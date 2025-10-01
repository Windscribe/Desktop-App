#include "execute_cmd.h"
#include <boost/thread.hpp>
#include <syslog.h>

unsigned long ExecuteCmd::execute(const std::string &cmd, const std::string &cwd, bool deleteOnFinish)
{
    mutex_.lock();

    curCmdId_++;
    CmdDescr *cmdDescr = new CmdDescr();
    cmdDescr->bFinished = false;
    cmdDescr->bSuccess = false;
    cmdDescr->cmdId = curCmdId_;
    executingCmds_.push_back(cmdDescr);
    mutex_.unlock();

    if (!cwd.empty()) {
        boost::thread(runCmd, curCmdId_, "cd \"" + cwd + "\" && " + cmd, deleteOnFinish);
    } else {
        boost::thread(runCmd, curCmdId_, cmd, deleteOnFinish);
    }

    return curCmdId_;
}

void ExecuteCmd::getStatus(unsigned long cmdId, bool &bFinished, std::string &log)
{
    mutex_.lock();
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it) {
        if ((*it)->cmdId == cmdId) {
            bFinished = (*it)->bFinished;
            log = (*it)->log;

            if ((*it)->bFinished) {
                delete (*it);
                executingCmds_.erase(it);
            }
            break;
        }
    }
    mutex_.unlock();
}

void ExecuteCmd::clearCmds()
{
    mutex_.lock();
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it) {
        delete (*it);
    }
    executingCmds_.clear();
    mutex_.unlock();
}


ExecuteCmd::ExecuteCmd() : curCmdId_(0)
{
}

void ExecuteCmd::runCmd(unsigned long cmdId, std::string cmd, bool deleteOnFinish)
{
    std::string strReply;

    FILE *file = popen(cmd.c_str(), "r");
    if (file) {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0) {
            if (instance().isCmdExist(cmdId)) {
                strReply += szLine;
            }
        }
        pclose(file);
        instance().cmdFinished(cmdId, true, strReply, deleteOnFinish);
    } else {
        instance().cmdFinished(cmdId, false, std::string(), deleteOnFinish);
    }
}

void ExecuteCmd::cmdFinished(unsigned long cmdId, bool bSuccess, std::string log, bool del)
{
    mutex_.lock();
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it) {
        if ((*it)->cmdId == cmdId) {
            if (del) {
                delete(*it);
                executingCmds_.erase(it);
            } else {
                (*it)->bFinished = true;
                (*it)->bSuccess = bSuccess;
                (*it)->log = log;
            }
            break;
        }
    }
    mutex_.unlock();
}

bool ExecuteCmd::isCmdExist(unsigned long cmdId)
{
    mutex_.lock();
    bool bFound = false;
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it) {
        if ((*it)->cmdId == cmdId) {
            bFound = true;
            break;
        }
    }
    mutex_.unlock();
    return bFound;
}
