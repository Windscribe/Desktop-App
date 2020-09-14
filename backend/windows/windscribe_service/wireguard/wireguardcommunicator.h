#pragma once

#include <string>

class WireGuardCommunicator final
{
public:
    WireGuardCommunicator() = default;

    void setDeviceName(const std::wstring &deviceName);
    bool configure(const std::string &clientPrivateKey, const std::string &peerPublicKey,
                   const std::string &peerPresharedKey, const std::string &peerEndpoint,
                   const std::vector<std::string> &allowedIps) const;
    UINT getStatus(UINT32 *errorCode, UINT64 *bytesReceived, UINT64 *bytesTransmitted) const;
    bool bindSockets(UINT if4, UINT if6, BOOL if6blackhole) const;

    static constexpr UINT SOCKET_INTERFACE_KEEP = ~UINT(0);

private:
    using ResultMap = std::map<std::string, std::string>;
    enum class OpenConnectionState { OK, NO_PIPE, NO_ACCESS };

    OpenConnectionState openConnection(FILE **pfp) const;
    bool getConnectionOutput(FILE *conn, ResultMap *results_map) const;

    std::wstring deviceName_;
};
