#pragma once

class WireGuardController final
{
public:
    explicit WireGuardController();

    bool installService(const std::wstring &exeName, const std::wstring &configFile);
    bool deleteService();

    UINT getStatus(UINT64& lastHandshake, UINT64& txBytes, UINT64& rxBytes) const;

private:
    bool is_initialized_ = false;

    std::string serviceName_;
    std::wstring deviceName_;
    std::wstring exeName_;

private:
    HANDLE getKernelInterfaceHandle() const;
};
