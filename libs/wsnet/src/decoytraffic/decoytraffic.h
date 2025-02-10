#pragma once

#include "WSNetDecoyTraffic.h"
#include <boost/asio.hpp>
#include "WSNetHttpNetworkManager.h"
#include "traffictrend.h"

namespace wsnet {

enum class FakeTrafficType { kLow = 0, kMedium, kHigh };

// Thread safe
class DecoyTraffic : public WSNetDecoyTraffic
{

public:
    explicit DecoyTraffic(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager);
    virtual ~DecoyTraffic();

    void setFakeTrafficVolume(std::uint32_t volume) override;
    void start() override;
    void stop() override;

private:
    std::mutex mutex_;
    boost::asio::io_context &io_context_;
    WSNetHttpNetworkManager *httpNetworkManager_;
    TrafficTrend trafficTrend_;
    std::uint32_t fakeTrafficVolume_;
    FakeTrafficType fakeTraffic_;

    boost::asio::steady_timer timer_;
    bool bStarted_ = false;

    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;
    std::chrono::time_point<std::chrono::steady_clock> lastRequestSendTime_;
    std::uint32_t sendTrafficIntervalInSeconds_ = 1;

    std::chrono::time_point<std::chrono::steady_clock> startTime_;
    std::uint64_t download_;
    std::uint64_t upload_;

    void job(bool skipCalc, const boost::system::error_code &err);

    void sendRequest(unsigned int dataToSendSize, unsigned int dataToReceiveSize);
    void onFinishedRequest(std::uint64_t requestId, std::uint32_t elapsedMs, NetworkError errCode, const std::string &curlError, const std::string &data,
                           unsigned int dataToSendSize, unsigned int dataToReceiveSize);

    std::uint32_t toBytes(FakeTrafficType fakeTraffic) const;
    std::uint32_t toMultiplier(FakeTrafficType fakeTraffic) const;
    std::uint32_t toInterval(FakeTrafficType fakeTraffic) const;
};


} // namespace wsnet
