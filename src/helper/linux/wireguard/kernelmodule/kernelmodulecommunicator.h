#pragma once

#include <map>
#include <string>
#include <vector>

#include "../iwireguardcommunicator.h"
#include "wireguard.h"

class KernelModuleCommunicator: public IWireGuardCommunicator
{
public:
    KernelModuleCommunicator() = default;

    virtual bool start(const std::string &deviceName);
    virtual bool stop();
    virtual bool configure(
        const std::string &clientPrivateKey,
        const std::string &peerPublicKey,
        const std::string &peerPresharedKey,
        const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps,
        uint32_t fwmark,
        uint16_t listenPort);
    virtual unsigned long getStatus(
        unsigned int *errorCode,
        unsigned long long *bytesReceived,
        unsigned long long *bytesTransmitted);

    static bool forceStop(const std::string &deviceName);

private:
    bool setPeerAllowedIps(wg_peer *peer, const std::vector<std::string> &ips);
    bool setPeerEndpoint(wg_peer *peer, const std::string &endpoint);
    void freeAllowedIps(wg_allowedip *ips);
    std::string deviceName_;
};
