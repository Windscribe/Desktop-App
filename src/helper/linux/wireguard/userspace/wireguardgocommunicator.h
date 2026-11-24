#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../../common/wireguardcommunicator_posix.h"
#include "../iwireguardcommunicator.h"

class WireGuardGoCommunicator: public IWireGuardCommunicator
{
public:
    bool start(const std::string &deviceName, bool verboseLogging) override;
    bool stop() override;
    bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
                   const std::string &peerPresharedKey, const std::string &peerEndpoint,
                   const std::vector<std::string> &allowedIps, uint32_t fwmark,
                   uint16_t listenPort, const AmneziawgConfig &amneziawgConfig, const std::string &persistentKeepalive,
                   bool disconnectOnStaleHandshake) override;
    unsigned long getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                            unsigned long long *bytesTransmitted) override;

    static bool forceStop(const std::string &deviceName);

private:
    WireGuardCommunicator comm_;
};
