#pragma once

#include <Windows.h>
#include <string>

struct AmneziawgConfig;

class WireGuardController final
{
public:
    static WireGuardController &instance()
    {
        static WireGuardController wc;
        return wc;
    }

    bool installService(bool isAmneziaWG);
    // Validates the fields and writes the service .conf. The caller never supplies config text.
    bool configure(const std::wstring &clientPrivateKey, const std::wstring &clientIpAddress,
                   const std::wstring &clientDnsAddress, const std::wstring &clientListenPort,
                   const std::wstring &peerPublicKey, const std::wstring &peerPresharedKey,
                   const std::wstring &peerEndpoint, const std::wstring &allowedIps,
                   const std::wstring &persistentKeepalive, const AmneziawgConfig &amneziawgConfig);
    bool deleteService();

    UINT getStatus(ULONG64 &lastHandshake, ULONG64 &txBytes, ULONG64 &rxBytes) const;

private:
    bool isInitialized_ = false;
    bool isAmneziaWG_ = false;
    std::wstring exeName_;

private:
    explicit WireGuardController();
    std::wstring configFile() const;

    void getKernelDriverStatus(ULONG64& lastHandshake, ULONG64& txBytes, ULONG64& rxBytes) const;
    HANDLE getKernelInterfaceHandle() const;

    void getAmneziaWGStatus(ULONG64& lastHandshake, ULONG64& txBytes, ULONG64& rxBytes) const;
};
