#include "kext_client.h"
#include "logger.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libproc.h>

#define KEXT_MSG_REPLY        0

KextClient::KextClient(const CheckAppCallback &funcCheckApp) : sock_(-1), isConnected_(false), thread_(NULL), funcCheckApp_(funcCheckApp)
{
}

KextClient::~KextClient()
{
    disconnect();
    kextMonitor_.unloadKext();
}

void KextClient::setKextPath(const std::string &kextPath)
{
    kextMonitor_.setKextPath(kextPath);
}

void KextClient::connect()
{
    if (isConnected_)
    {
        return;
    }
    
    if (!kextMonitor_.loadKext())
    {
        return;
    }
    
    sock_ = ::socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if(sock_ < 0)
    {
        LOG("Socket create error");
        return;
    }
    
    // Set close on exec flag, to prevent socket being inherited by
    // child processes (such as openvpn). If we fail to do this
    // the Kext thinks it stil has an open connection after we close it here,
    // due to the open file descriptor in the openvpn sub process
    if(::fcntl(sock_, F_SETFD, FD_CLOEXEC))
    {
        LOG("fcntl error");
    }
    
    ctl_info ctl_info = {};
    strncpy(ctl_info.ctl_name, "com.windscribe.kext", sizeof(ctl_info.ctl_name));

    if (::ioctl(sock_, CTLIOCGINFO, &ctl_info) == -1)
    {
        LOG("ioctl error");
        ::close(sock_);
        return;
    }
    
    sockaddr_ctl sc = {};

    sc.sc_len = sizeof(sockaddr_ctl);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = SYSPROTO_CONTROL;
    sc.sc_id = ctl_info.ctl_id;
    sc.sc_unit = 0;

    if(::connect(sock_, reinterpret_cast<sockaddr*>(&sc), sizeof(sockaddr_ctl)))
    {
        LOG("Connect error");
        ::close(sock_);
        return;
    }
    
    thread_ = new std::thread(&KextClient::readSocketThread, this);
    
    LOG("Socket successfully connected");
    isConnected_ = true;
}

void KextClient::disconnect()
{
    if (isConnected_)
    {
        shutdown(sock_, SHUT_RDWR);
        
        if (thread_)
        {
            thread_->join();
            delete thread_;
            thread_ = NULL;
        }
        
        close(sock_);
        isConnected_ = false;
        sock_ = -1;
        kextMonitor_.unloadKext();
        LOG("Socket disconnected");
    }
}

void KextClient::readSocketThread()
{
    ProcQuery procQuery = {};
    ProcQuery procResponse = {};

    while (recvAll(sock_, &procQuery, sizeof(procQuery)))
    {
        LOG("packet received");
        
        procResponse.id = procQuery.id;
        procResponse.command = procQuery.command;
        procResponse.pid = procQuery.pid;
        procResponse.accept = false;
        
        if (procQuery.needs_reply)
        {
            
            proc_pidpath(procQuery.pid, procResponse.app_path, sizeof(procResponse.app_path));
            
            std::string s(procResponse.app_path);
         
            std::string outIp;
            bool isExclude;
            procResponse.accept = funcCheckApp_(s, outIp, isExclude);
            procResponse.rule_type = isExclude ? BypassVPN : OnlyVPN;
            if (procResponse.accept)
            {
                procResponse.bind_ip = inet_addr(outIp.c_str());
            }
            
            // Send reply back to Kext using setsockopt()  - ::send() turned out to be unreliable
            setsockopt(sock_, SYSPROTO_CONTROL, KEXT_MSG_REPLY, &procResponse, sizeof(procResponse));
            
            LOG("answer sent");
        }
    }
}

bool KextClient::recvAll(int socket, void *buffer, size_t length)
{
    char *ptr = (char*) buffer;
    while (length > 0)
    {
        size_t i = recv(socket, ptr, length, 0);
        if (i < 1) return false;
        ptr += i;
        length -= i;
    }
    return true;
}
