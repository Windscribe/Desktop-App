#pragma once

#include <set>
#include <string>
#include <vector>

#include "fwpm_wrapper.h"
#include "types/ipaddress.h"

class DnsFirewall
{
public:
    static DnsFirewall &instance(FwpmWrapper *wrapper = nullptr)
    {
        static DnsFirewall df(*wrapper);
        return df;
    }

    void release();

    void disable();
    void enable();

    void setExcludeIps(const std::vector<std::wstring>& ips);

private:
    bool bCurrentState_;
    GUID subLayerGUID_;
    FwpmWrapper &fwmpWrapper_;
    std::set<types::IpAddress> excludeIps_;

    explicit DnsFirewall(FwpmWrapper &fwmpWrapper);
    void addFilters(HANDLE engineHandle);
    std::vector<types::IpAddress> getDnsServers();
};
