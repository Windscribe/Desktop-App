#include "helper_mac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../../../../../backend/mac/helper/src/ipc/helper_commands_serialize.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

Helper_mac::Helper_mac() : sock_(-1)
{
}

Helper_mac::~Helper_mac()
{
    stop();
}

bool Helper_mac::connect()
{
    struct sockaddr_un addr;
    sock_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_ < 0)
    {
        sock_ = -1;
        return false;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path)-1);
    
    if (::connect(sock_, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        close(sock_);
        sock_ = -1;
        return false;
    }
    
    return true;
}

void Helper_mac::stop()
{
    if (sock_ >= 0)
    {
        close(sock_);
    }
}

bool Helper_mac::setPaths(const std::wstring &archivePath, const std::wstring &installPath, uid_t userId, gid_t groupId)
{
    CMD_INSTALLER_FILES_SET_PATH cmd;
    cmd.archivePath = archivePath;
    cmd.installPath = installPath;
    cmd.userId = userId;
    cmd.groupId = groupId;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;
    if (!sendCmdToHelper(HELPER_CMD_INSTALLER_SET_PATH, stream.str()))
    {
        return false;
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd))
    {
        return false;
    }
    
    return answerCmd.executed == 1;
}

int Helper_mac::executeFilesStep()
{
    if (!sendCmdToHelper(HELPER_CMD_INSTALLER_EXECUTE_COPY_FILE, std::string()))
    {
        return false;
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd))
    {
        return false;
    }
    
    return answerCmd.executed;
}

std::string Helper_mac::executeRootCommand(const std::string &commandLine, bool &bSuccess)
{
    CMD_EXECUTE cmd;
    cmd.cmdline = commandLine;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;
    
    if (!sendCmdToHelper(HELPER_CMD_EXECUTE, stream.str()))
    {
        return "";
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd))
    {
        return "";
    }
    bSuccess = true;
    return answerCmd.body;
}

bool Helper_mac::killProcess(pid_t pid)
{
    CMD_KILL_PROCESS cmd;
    cmd.processId = pid;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;
    
    if (!sendCmdToHelper(HELPER_CMD_KILL_PROCESS, stream.str()))
    {
        return "";
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd))
    {
        return "";
    }
    
    return answerCmd.executed != 0;
}

bool Helper_mac::sendCmdToHelper(int cmdId, const std::string &data)
{
    int length = (int)data.size();
    if (!sendAll(sock_, (void *)&cmdId, sizeof(cmdId)))
    {
        return false;
    }
    
    // second 4 bytes - pid
    const auto pid = getpid();
    if (!sendAll(sock_, (void *)&pid, sizeof(pid)))
    {
        return false;
    }
    
    // third 4 bytes - size of buffer
    if (!sendAll(sock_, (void *)&length, sizeof(length)))
    {
        return false;
    }

    // body of message
    if (length > 0)
    {
        if (!sendAll(sock_, (void *)data.data(), length))
        {
            return false;
        }
    }
    
    return true;
}

bool Helper_mac::readAnswer(CMD_ANSWER &outAnswer)
{
    int length;
    if (!recvAll(sock_, &length, sizeof(length)))
    {
        return false;
    }
    std::vector<char> buff(length);
    if (!recvAll(sock_, (void *)&buff[0], length))
    {
        return false;
    }
    
    std::string str(buff.begin(), buff.end());
    std::istringstream stream(str);
    boost::archive::text_iarchive ia(stream, boost::archive::no_header);
    ia >> outAnswer;
    return true;
}

bool Helper_mac::sendAll(int s, void *buf, int len)
{
    const char *data_ptr = (const char*) buf;
    ssize_t bytes_sent;

    while (len > 0)
    {
        bytes_sent = ::send(s, data_ptr, len, 0);
        if (bytes_sent == -1)
            return false;

        data_ptr += bytes_sent;
        len -= bytes_sent;
    }

    return true;
}

bool Helper_mac::recvAll(int s, void *buf, int len)
{
    char *data_ptr = (char *) buf;
    ssize_t bytes_recv;
    while (len > 0)
    {
        bytes_recv = ::read(s, data_ptr, len);
        if (bytes_recv == -1)
            return false;

        data_ptr += bytes_recv;
        len -= bytes_recv;
    }

    return true;
}
