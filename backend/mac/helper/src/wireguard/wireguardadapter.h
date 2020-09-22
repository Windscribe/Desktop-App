#ifndef WireGuardAdapter_h
#define WireGuardAdapter_h

#include <map>
#include <string>
#include <vector>

class WireGuardAdapter final
{
public:
    explicit WireGuardAdapter(const std::string &name);
    ~WireGuardAdapter();

    bool setIpAddress(const std::string &address);
    bool setDnsServers(const std::string &addressList);
    bool enableRouting(const std::vector<std::string> &allowedIps);
    const std::string getName() const { return name_; }
    bool hasDefaultRoute() const { return has_default_route_; }

private:
    void initializeOnce();
    bool flushDnsServer();

    std::string name_;
    std::map<std::string,std::string> dns_info_;
    bool is_adapter_initialized_;
    bool has_default_route_;
};

#endif  // WireGuardAdapter_h
