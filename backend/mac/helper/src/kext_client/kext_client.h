#ifndef KextClient_h
#define KextClient_h

#include <thread>
#include "kext_monitor.h"
#include "../ipc/helper_commands.h"

class KextClient
{
public:
    explicit KextClient();
    ~KextClient();
    
    void setKextPath(const std::string &kextPath);
    void connect();
    void disconnect();
    
    void setConnectParams(CMD_SEND_CONNECT_STATUS &connectStatus);
    void setSplitTunnelingParams(bool isExclude, const std::vector<std::string> &apps);
    
private:
    
    enum CommandType
    {
        VerifyApp         // check whether the app should be excluded from VPN
    };
    
    enum RuleType
    {
        BypassVPN,
        OnlyVPN
    };
    
    typedef struct ProcQuery
    {
        enum CommandType command;
        char needs_reply;
        char app_path[PATH_MAX];
        int pid;
        int accept;
        enum RuleType rule_type;
        uint32_t id;
        uint32_t source_ip;
        uint32_t source_port;
        uint32_t dest_ip;
        uint32_t dest_port;
        uint32_t bind_ip;   // the IP to bind to
        
        // SOCK_STREAM or SOCK_DGRAM (tcp or udp)
        int socket_type;
    } ProcQuery;
    
    std::mutex mutex_;
    
    int sock_;
    bool isConnected_;
    
    std::thread *thread_;
    std::atomic<bool> bFinishThread_;
    KextMonitor kextMonitor_;
    
    bool isExclude_;
    CMD_SEND_CONNECT_STATUS connectStatus_;
    std::vector<std::string> windscribeExecutables_;
    std::vector<std::string> apps_;
    
    const std::string WEBKIT_FRAMEWORK_PATH = std::string("/System/Library/Frameworks/WebKit.framework");
    const std::string STAGED_WEBKIT_FRAMEWORK_PATH = std::string("/System/Library/StagedFrameworks/Safari/WebKit.framework");
    
    void readSocketThread();
    bool recvAll(int socket, void *buffer, size_t length);
    
    void applyExtraRules(std::vector<std::string> &apps);
    bool verifyApp(const std::string &appPath, std::string &outBindIp, bool &isExclude);
};

#endif // KextClient_h
