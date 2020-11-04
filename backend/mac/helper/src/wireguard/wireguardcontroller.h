#ifndef WireGuardController_h
#define WireGuardController_h

#include <memory>
#include <string>
#include <vector>

class DefaultRouteMonitor;
class WireGuardAdapter;
class WireGuardCommunicator;

class WireGuardController
{
public:
    WireGuardController();

    void init(const std::string &deviceName, unsigned long daemonCmdId);
    void reset();

    bool configureAdapter(const std::string &ipAddress, const std::string &dnsAddressList,
        const std::vector<std::string> &allowedIps);
    const std::string getAdapterName() const;
    bool configureDefaultRouteMonitor(const std::string &peerEndpoint);
    bool configureDaemon(const std::string &clientPrivateKey, const std::string &peerPublicKey,
        const std::string &peerPresharedKey, const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps);
    bool isInitialized() const { return is_initialized_; }
    unsigned long getDaemonCmdId() const { return daemonCmdId_; }
    unsigned long getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                            unsigned long long *bytesTransmitted) const;

    static std::vector<std::string> splitAndDeduplicateAllowedIps(const std::string &allowedIps);

private:
    std::unique_ptr<WireGuardAdapter> adapter_;
    std::unique_ptr<DefaultRouteMonitor> drm_;
    std::shared_ptr<WireGuardCommunicator> comm_;
    unsigned long daemonCmdId_;
    bool is_initialized_;
};

#endif  // WireGuardController_h
