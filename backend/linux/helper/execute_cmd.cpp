#include "execute_cmd.h"
#include <boost/thread.hpp>
#include <syslog.h>

unsigned long ExecuteCmd::execute(const char *cmd)
{
    mutex_.lock();
    curCmdId_++;
    CmdDescr *cmdDescr = new CmdDescr();
    cmdDescr->bFinished = false;
    cmdDescr->bSuccess = false;
    cmdDescr->cmdId = curCmdId_;
    executingCmds_.push_back(cmdDescr);
    mutex_.unlock();
    
    std::string str = cmd;
    boost::thread(runCmd, curCmdId_, str);
    
    return curCmdId_;
}

void ExecuteCmd::getStatus(unsigned long cmdId, bool &bFinished, std::string &log)
{
    mutex_.lock();
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it)
    {
        if ((*it)->cmdId == cmdId)
        {
            bFinished = (*it)->bFinished;
            log = (*it)->log;
            
            if ((*it)->bFinished)
            {
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
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it)
    {
        delete (*it);
    }
    executingCmds_.clear();
    mutex_.unlock();
}


ExecuteCmd::ExecuteCmd() : curCmdId_(0)
{
    
}

void ExecuteCmd::runCmd(unsigned long cmdId, std::string cmd)
{
    std::string strReply;
     
    // run openvpn command
    FILE *file = popen(cmd.c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            if (instance().isCmdExist(cmdId))
            {
                strReply += szLine;
            }
        }
        pclose(file);
        instance().cmdFinished(cmdId, true, strReply);
    }
    else
    {
        instance().cmdFinished(cmdId, false, std::string());
    }
}

void ExecuteCmd::cmdFinished(unsigned long cmdId, bool bSuccess, std::string log)
{
    mutex_.lock();
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it)
    {
        if ((*it)->cmdId == cmdId)
        {
            (*it)->bFinished = true;
            (*it)->bSuccess = bSuccess;
            (*it)->log = log;
            break;
        }
    }
    mutex_.unlock();
}

bool ExecuteCmd::isCmdExist(unsigned long cmdId)
{
    mutex_.lock();
    bool bFound = false;
    for (auto it = executingCmds_.begin(); it != executingCmds_.end(); ++it)
    {
        if ((*it)->cmdId == cmdId)
        {
            bFound = true;
            break;
        }
    }
    mutex_.unlock();
    return bFound;
}
