#include "wireguardgocommunicator.h"

bool WireGuardGoCommunicator::start(const std::string &deviceName, bool verboseLogging)
{
    return comm_.start(deviceName, verboseLogging);
}

bool WireGuardGoCommunicator::stop()
{
    return comm_.stop();
}

bool WireGuardGoCommunicator::configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
                                        const std::string &peerPresharedKey, const std::string &peerEndpoint,
                                        const std::vector<std::string> &allowedIps, uint32_t fwmark,
                                        uint16_t listenPort, const AmneziawgConfig &amneziawgConfig, const std::string &persistentKeepalive,
                                        bool disconnectOnStaleHandshake)
{
    return comm_.configure(clientPrivateKey, peerPublicKey, peerPresharedKey, peerEndpoint, allowedIps, fwmark, listenPort,
                           amneziawgConfig, persistentKeepalive, disconnectOnStaleHandshake);
}

unsigned long WireGuardGoCommunicator::getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                                                 unsigned long long *bytesTransmitted)
{
    return comm_.getStatus(errorCode, bytesReceived, bytesTransmitted);
}

// static
bool WireGuardGoCommunicator::forceStop(const std::string &deviceName)
{
    return WireGuardCommunicator::forceStop(deviceName);
}
