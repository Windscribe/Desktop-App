#ifndef KextClient_h
#define KextClient_h

#include <thread>
#include "kext_monitor.h"

typedef std::function<bool(const std::string&, std::string&, bool&)> CheckAppCallback;

class KextClient
{
public:
    KextClient(const CheckAppCallback &funcCheckApp);
    ~KextClient();
    
    void setKextPath(const std::string &kextPath);
    void connect();
    void disconnect();
    
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
    
    
    int sock_;
    bool isConnected_;
    
    std::thread *thread_;
    CheckAppCallback funcCheckApp_;
    KextMonitor kextMonitor_;
    
    void readSocketThread();
    bool recvAll(int socket, void *buffer, size_t length);
    
};

#endif // KextClient_h
