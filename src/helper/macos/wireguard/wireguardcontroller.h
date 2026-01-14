#pragma once

#include <memory>
#include <string>
#include <vector>

#include "defaultroutemonitor.h"
#include "wireguardadapter.h"
#include "wireguardcommunicator.h"


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

    bool configureAdapter(const std::string &ipAddress, const std::string &dnsAddressList,
        const std::string &dnsScriptName, const std::vector<std::string> &allowedIps);
    bool configureDefaultRouteMonitor(const std::string &peerEndpoint);
    bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
        const std::string &peerPresharedKey, const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps, uint16_t listenPort);
    bool isInitialized() const { return is_initialized_; }
    unsigned long getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                            unsigned long long *bytesTransmitted) const;

    static std::vector<std::string> splitAndDeduplicateAllowedIps(const std::string &allowedIps);

private:
    inline static const std::string kAdapterName = "utun420";

    std::unique_ptr<WireGuardAdapter> adapter_;
    std::unique_ptr<DefaultRouteMonitor> drm_;
    std::shared_ptr<WireGuardCommunicator> comm_;
    bool is_initialized_;

    WireGuardController();
    ~WireGuardController() {};
};
