#pragma once

#include <string>

class WireGuardCommunicator final
{
public:
    WireGuardCommunicator() = default;

    void setDeviceName(const std::wstring &deviceName);
    bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
                   const std::string &peerPresharedKey, const std::string &peerEndpoint,
                   const std::vector<std::string> &allowedIps);
    UINT getStatus(UINT32 *errorCode, UINT64 *bytesReceived, UINT64 *bytesTransmitted);
    bool bindSockets(UINT if4, UINT if6, BOOL if6blackhole);
    void quit();

    static constexpr UINT SOCKET_INTERFACE_KEEP = ~UINT(0);

private:
    class Connection
    {
    public:
        enum class Status { OK, NO_PIPE, NO_ACCESS };
        using ResultMap = std::map<std::string, std::string>;

        explicit Connection(const std::wstring deviceName);
        ~Connection();
        bool getOutput(ResultMap *results_map) const;
        Status getStatus() const { return status_; }
        operator FILE*() const { return fileHandle_; }
    private:
        Status status_;
        HANDLE pipeHandle_;
        FILE *fileHandle_;
    };

    std::wstring deviceName_;
};
