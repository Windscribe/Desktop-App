#pragma once

#include <memory>
#include <string>
#include <vector>

#include "iwireguardcommunicator.h"
#include "defaultroutemonitor.h"
#include "wireguardadapter.h"

class WireGuardController
{
public:
    static WireGuardController &instance()
    {
        static WireGuardController wgc;
        return wgc;
    }

    bool start();
    bool stop();

    bool configure(
        const std::string &clientPrivateKey,
        const std::string &peerPublicKey,
        const std::string &peerPresharedKey,
        const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps,
        uint32_t fwmark,
        uint16_t listenPort);
    unsigned long getStatus(
        unsigned int *errorCode,
        unsigned long long *bytesReceived,
        unsigned long long *bytesTransmitted) const;

    bool configureAdapter(
        const std::string &ipAddress,
        const std::string &dnsAddressList,
        const std::string &dnsScriptName,
        const std::vector<std::string> &allowedIps,
        uint32_t fwmark);
    std::string getAdapterName() const;
    bool configureDefaultRouteMonitor(const std::string &peerEndpoint);

    bool isInitialized() const { return is_initialized_; }

    static std::vector<std::string> splitAndDeduplicateAllowedIps(
        const std::string &allowedIps);
    static uint32_t getFwmark();

private:
    inline static std::string kDeviceName = "utun420";

    std::unique_ptr<WireGuardAdapter> adapter_;
    std::unique_ptr<DefaultRouteMonitor> drm_;
    std::shared_ptr<IWireGuardCommunicator> comm_;
    bool is_initialized_;

    WireGuardController();
};
