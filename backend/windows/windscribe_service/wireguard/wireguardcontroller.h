#pragma once

#include "../firewallfilter.h"

class WireGuardController final
{
public:
    explicit WireGuardController(FirewallFilter &firewallFilter);

    bool installService(const std::wstring &exeName, const std::wstring &configFile);
    bool deleteService();

    UINT getStatus(UINT64& lastHandshake, UINT64& txBytes, UINT64& rxBytes) const;

private:
    FirewallFilter const& firewallFilter_;
    bool is_initialized_;

    std::string serviceName_;
    std::wstring deviceName_;

private:
    HANDLE getKernelInterfaceHandle() const;
};
