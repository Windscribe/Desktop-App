#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "fwpm_wrapper.h"

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
    std::unordered_set<std::wstring> excludeIps_;

    explicit DnsFirewall(FwpmWrapper &fwmpWrapper);
    void addFilters(HANDLE engineHandle);
    std::vector<std::wstring> getDnsServers();
};
