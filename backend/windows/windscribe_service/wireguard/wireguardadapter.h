#pragma once

#include <string>

class WireGuardAdapter final
{
public:
    explicit WireGuardAdapter(const std::wstring &name);
    ~WireGuardAdapter();

    bool setIpAddress(const std::string &address, bool isDefault);
    bool setDnsServers(const std::string &addressList);
    bool enableRouting(const std::vector<std::string> &allowedIps);
    NET_LUID getLuid() const { return luid_; }

private:
    void initializeOnce();
    bool unsetIpAddress();
    bool flushDnsServer();
    bool clearDnsDomain();
    bool disableRouting();

    NET_LUID luid_;
    GUID guid_;
    std::wstring name_;
    std::wstring guidString_;
    bool is_adapter_initialized_;
    bool is_routing_enabled_;
};
