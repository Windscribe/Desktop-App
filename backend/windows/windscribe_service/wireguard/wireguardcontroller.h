#pragma once

#include <memory>

class DefaultRouteMonitor;
class WireGuardAdapter;
class WireGuardCommunicator;

class WireGuardController final
{
public:
    WireGuardController();

    void init(const std::wstring &deviceName, UINT daemonCmdId);
    void reset();

    bool configureAdapter(const std::string &ipAddress, const std::string &dnsAddressList,
                          const std::vector<std::string> &allowedIps);
    bool configureDefaultRouteMonitor();
    bool configureDaemon(const std::string &clientPrivateKey, const std::string &peerPublicKey,
                         const std::string &peerPresharedKey, const std::string &peerEndpoint,
                         const std::vector<std::string> &allowedIps);
    bool isInitialized() const { return is_initialized_; }
    UINT getDaemonCmdId() const { return daemonCmdId_; }
    UINT getStatus(UINT32 *errorCode, UINT64 *bytesReceived, UINT64 *bytesTransmitted) const;

    static std::vector<std::string> splitAndDeduplicateAllowedIps(const std::string &allowedIps);

private:
    std::unique_ptr<WireGuardAdapter> adapter_;
    std::unique_ptr<DefaultRouteMonitor> drm_;
    std::shared_ptr<WireGuardCommunicator> comm_;
    UINT daemonCmdId_;
    bool is_initialized_;
};
