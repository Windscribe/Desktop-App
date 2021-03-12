#include "kext_client.h"
#include "logger.h"
#include "utils.h"
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

KextClient::KextClient() : sock_(-1), isConnected_(false), thread_(NULL), bFinishThread_(false)
{
    isExclude_ = false;
    connectStatus_.isConnected = false;
    
    windscribeExecutables_.push_back("wsappcontrol");
    windscribeExecutables_.push_back("/WindscribeEngine.app");
}

KextClient::~KextClient()
{
    disconnect();
    kextMonitor_.unloadKext();
}

void KextClient::setKextPath(const std::string &kextPath)
{
    std::lock_guard<std::mutex> guard(mutex_);
    kextMonitor_.setKextPath(kextPath);
}

void KextClient::connect()
{
    std::lock_guard<std::mutex> guard(mutex_);
    
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
        LOG("Connect error: %d", errno);
        ::close(sock_);
        return;
    }
    
    bFinishThread_ = false;
    thread_ = new std::thread(&KextClient::readSocketThread, this);
    
    LOG("Socket successfully connected");
    isConnected_ = true;
}

void KextClient::disconnect()
{
    if (isConnected_)
    {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            close(sock_);
        }
        
        if (thread_)
        {
            thread_->join();
            delete thread_;
            thread_ = NULL;
        }

        isConnected_ = false;
        sock_ = -1;
        kextMonitor_.unloadKext();
        LOG("Socket disconnected");
    }
}

void KextClient::setConnectParams(CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::lock_guard<std::mutex> guard(mutex_);
    connectStatus_ = connectStatus;
}

void KextClient::setSplitTunnelingParams(bool isExclude, const std::vector<std::string> &apps)
{
    std::lock_guard<std::mutex> guard(mutex_);
    isExclude_ = isExclude;
    apps_ = apps;
    applyExtraRules(apps_);
}

void KextClient::readSocketThread()
{
    ProcQuery procQuery = {};
    ProcQuery procResponse = {};

    while (recvAll(sock_, &procQuery, sizeof(procQuery)))
    {
        std::lock_guard<std::mutex> guard(mutex_);
        
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
            procResponse.accept = verifyApp(s, outIp, isExclude);
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
        ssize_t i = recv(socket, ptr, length, 0);
        if (i < 1) return false;
        ptr += i;
        length -= i;
    }
    return true;
}

// add extra paths for Mac standard applications
void KextClient::applyExtraRules(std::vector<std::string> &apps)
{
    std::vector<std::string> addedPaths;
    
    // If the system WebKit framework is excluded/vpnOnly, add this staged framework
    // path too. Newer versions of Safari use this.
    for (const auto &path: apps)
    {
        if (Utils::findCaseInsensitive(path, "/App Store.app") != std::string::npos)
        {
            addedPaths.push_back(WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back(STAGED_WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back("/System/Library/PrivateFrameworks/AppStoreDaemon.framework/Support/appstoreagent");
        }
        else if (Utils::findCaseInsensitive(path, "/Calendar.app") != std::string::npos)
        {
            addedPaths.push_back(WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back(STAGED_WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back("/System/Library/PrivateFrameworks/CalendarAgent.framework/Executables/CalendarAgent");
        }
        else if (Utils::findCaseInsensitive(path, "/Safari.app") != std::string::npos)
        {
            addedPaths.push_back(WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back(STAGED_WEBKIT_FRAMEWORK_PATH);
            
            addedPaths.push_back("/System/Library/CoreServices/SafariSupport.bundle/Contents/MacOS/SafariBookmarksSyncAgent");
            addedPaths.push_back("/System/Library/StagedFrameworks/Safari/WebKit.framework/Versions/A/XPCServices/com.apple.WebKit.Networking.xpc");
            addedPaths.push_back("/System/Library/PrivateFrameworks/SafariSafeBrowsing.framework/Versions/A/com.apple.Safari.SafeBrowsing.Service");
            addedPaths.push_back("/System/Library/StagedFrameworks/Safari/SafariShared.framework/Versions/A/XPCServices/com.apple.Safari.SearchHelper.xpc");
        }
    }
    
    for (const auto &path: addedPaths)
    {
        apps.push_back(path);
    }
}
bool KextClient::verifyApp(const std::string &appPath, std::string &outBindIp, bool &isExclude)
{
    isExclude = isExclude_;
        
    if (connectStatus_.defaultAdapter.adapterIp.empty())
    {
        goto not_verify;
    }
        
    // check for Windscribe executables
    for (const auto &windscribePath: windscribeExecutables_)
    {
        if (Utils::findCaseInsensitive(appPath, windscribePath) != std::string::npos)
        {
            if (isExclude)
            {
                goto not_verify;
            }
            else
            {
                outBindIp = connectStatus_.vpnAdapter.adapterIp;
                LOG("VerifyApp: %s, result: %d", appPath.c_str(), true);
                return true;
            }
        }
    }
        
    // check for app list
    for (std::vector<std::string>::iterator it = apps_.begin(); it != apps_.end(); ++it)
    {
        if (appPath.rfind(*it, 0) == 0)
        {
            if (isExclude)
            {
                outBindIp = connectStatus_.defaultAdapter.adapterIp;
            }
            else
            {
                outBindIp = connectStatus_.vpnAdapter.adapterIp;
            }
            LOG("VerifyApp: %s, result: %d", appPath.c_str(), true);
            return true;
        }
    }
    
not_verify:
    LOG("VerifyApp: %s, result: %d", appPath.c_str(), false);
    return false;
}
