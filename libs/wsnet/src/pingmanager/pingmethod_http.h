#pragma once

#include "ipingmethod.h"
#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"

namespace wsnet {


class PingMethodHttp : public IPingMethod
{
public:
    PingMethodHttp(WSNetHttpNetworkManager *httpNetworkManager, std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
                    PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback, WSNetAdvancedParameters *advancedParameters);

    virtual ~PingMethodHttp();
    void ping(bool isFromDisconnectedVpnState) override;

private:
    enum { PING_TIMEOUT = 2000 };
    WSNetHttpNetworkManager *httpNetworkManager_;
    std::shared_ptr<WSNetCancelableCallback> request_;
    WSNetAdvancedParameters *advancedParameters_;

    void onNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, NetworkError errCode, const std::string &data);
    int parseReplyString(const std::string &data);
};

} // namespace wsnet
