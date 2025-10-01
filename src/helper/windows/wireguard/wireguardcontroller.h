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

    bool installService();
    bool configure(const std::wstring &config);
    bool deleteService();

    UINT getStatus(ULONG64 &lastHandshake, ULONG64 &txBytes, ULONG64 &rxBytes) const;

private:
    bool is_initialized_ = false;
    std::wstring exeName_;

private:
    explicit WireGuardController();
    HANDLE getKernelInterfaceHandle() const;
    std::wstring configFile() const;
};
