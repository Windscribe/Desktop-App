#ifndef WireGuardCommunicator_h
#define WireGuardCommunicator_h

#include <map>
#include <string>
#include <vector>

class WireGuardCommunicator
{
public:
    WireGuardCommunicator() = default;

    void setDeviceName(const std::string &deviceName);
    bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
        const std::string &peerPresharedKey, const std::string &peerEndpoint,
        const std::vector<std::string> &allowedIps, uint32_t fwmark);
    unsigned long getStatus(unsigned int *errorCode, unsigned long long *bytesReceived,
                            unsigned long long *bytesTransmitted);

private:
    class Connection
    {
    public:
        enum class Status { OK, NO_SOCKET, NO_ACCESS };
        using ResultMap = std::map<std::string, std::string>;

        explicit Connection(const std::string &deviceName);
        ~Connection();
        bool getOutput(ResultMap *results_map) const;
        Status getStatus() const { return status_; }
        operator FILE*() const { return fileHandle_; }
    private:
        bool connect(struct sockaddr_un *address);

        static constexpr int CONNECTION_ATTEMPT_COUNT = 5;
        static constexpr int CONNECTION_BETWEEN_WAIT_MS = 100;
        Status status_;
        int socketHandle_;
        FILE *fileHandle_;
    };

    std::string deviceName_;
};

#endif  // WireGuardCommunicator_h
