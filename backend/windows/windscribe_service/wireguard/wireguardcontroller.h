#pragma once

#include <Windows.h>
#include <string>

class WireGuardController final
{
public:
    static WireGuardController &instance()
    {
        static WireGuardController wc;
        return wc;
    }

    bool installService(const std::wstring &exeName, const std::wstring &configFile);
    bool deleteService();

    UINT getStatus(UINT64& lastHandshake, UINT64& txBytes, UINT64& rxBytes) const;

private:
    bool is_initialized_ = false;

    std::wstring deviceName_;
    std::wstring exeName_;

private:
    explicit WireGuardController();
    HANDLE getKernelInterfaceHandle() const;
};
