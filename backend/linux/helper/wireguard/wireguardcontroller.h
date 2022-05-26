#ifndef WireGuardController_h
#define WireGuardController_h

#include <memory>
#include <string>
#include <vector>

#include "iwireguardcommunicator.h"

class DefaultRouteMonitor;
class WireGuardAdapter;
class WireGuardCommunicator;

class WireGuardController
{
public:
    WireGuardController();

    bool start(
        const std::string &exePath,
        const std::string &deviceName);
    bool stop();

    bool configure(
        const std::string &clientPrivateKey,
        const std::string &peerPublicKey,
        const std::string &peerPresharedKey,
        const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps,
        uint32_t fwmark);
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

    bool isInitialized() const { return is_initialized_; }

    static std::vector<std::string> splitAndDeduplicateAllowedIps(
        const std::string &allowedIps);
    static uint32_t getFwmark();

private:
    std::unique_ptr<WireGuardAdapter> adapter_;
    std::shared_ptr<IWireGuardCommunicator> comm_;
    bool is_initialized_;
};

#endif  // WireGuardController_h
