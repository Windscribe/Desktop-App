#ifndef WireGuardCommunicator_h
#define WireGuardCommunicator_h

#include <map>
#include <string>

class WireGuardCommunicator
{
public:
    WireGuardCommunicator() = default;

    void setDeviceName(const std::string &deviceName);
    bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
        const std::string &peerPresharedKey, const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps) const;
    unsigned long getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                            unsigned long long *bytesTransmitted) const;

private:
    using ResultMap = std::map<std::string, std::string>;
    enum class OpenConnectionState { OK, NO_PIPE, NO_ACCESS };

    OpenConnectionState openConnection(FILE **pfp) const;
    bool getConnectionOutput(FILE *conn, ResultMap *results_map) const;

    std::string deviceName_;
};

#endif  // WireGuardCommunicator_h
