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
    bool setDnsServers(const std::string &addressList, const std::string &scriptName);
    bool enableRouting(const std::vector<std::string> &allowedIps);
    const std::string getName() const { return name_; }
    bool hasDefaultRoute() const { return has_default_route_; }

private:
    bool flushDnsServer();

    std::string name_;
    std::string dns_script_name_;
    bool is_dns_server_set_;
    bool has_default_route_;
};

#endif  // WireGuardAdapter_h
