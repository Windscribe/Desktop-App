#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/helper_commands.h"

class IWireGuardCommunicator
{
public:
    virtual bool start(const std::string &deviceName, bool verboseLogging) = 0;
    virtual bool stop() = 0;
    virtual bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
                           const std::string &peerPresharedKey, const std::string &peerEndpoint,
                           const std::vector<std::string> &allowedIps, uint32_t fwmark,
                           uint16_t listenPort,const AmneziawgConfig &amneziawgConfig) = 0;
    virtual unsigned long getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                                    unsigned long long *bytesTransmitted) = 0;
};
