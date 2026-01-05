#pragma once

#include <map>
#include <string>
#include <vector>

#include "../iwireguardcommunicator.h"

class WireGuardGoCommunicator: public IWireGuardCommunicator
{
public:
    WireGuardGoCommunicator() = default;

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
    std::string executable_;
    unsigned long daemonCmdId_;
};
