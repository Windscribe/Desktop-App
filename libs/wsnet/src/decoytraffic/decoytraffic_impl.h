#pragma once

#include <boost/asio.hpp>
#include "WSNetHttpNetworkManager.h"

namespace wsnet {

enum class FakeTrafficType { kLow = 0, kMedium, kHigh };

// Thread safe
class DecoyTraffic_impl
{

public:
    explicit DecoyTraffic_impl(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, bool isUpload);
    virtual ~DecoyTraffic_impl();

    void setFakeTrafficVolume(std::uint32_t volume);
    void start(std::uint32_t startIntervalSeconds);
    // return total upload/download Mbytes for the current session
    double stop();

private:
    std::mutex mutex_;
    boost::asio::io_context &io_context_;
    WSNetHttpNetworkManager *httpNetworkManager_;
    bool isUpload_;
    FakeTrafficType fakeTraffic_;
    boost::asio::steady_timer timer_;
    bool bStarted_ = false;
    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;
    std::chrono::time_point<std::chrono::steady_clock> lastRequestFinishedTime_;
    std::uint32_t sendTrafficIntervalInSeconds_ = 1;
    double bandwidth_;
    std::uint64_t total_;
    std::uint32_t initialSize_;
    std::uint32_t remainingSize_;

    void job(bool skipCalc, const boost::system::error_code &err);

    void sendRequest(uint32_t dataToSendSize, uint32_t dataToReceiveSize);
    void onFinishedRequest(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data,
                           unsigned int size, std::chrono::time_point<std::chrono::high_resolution_clock> startTime);

    std::uint32_t toInterval(FakeTrafficType fakeTraffic) const;
};


} // namespace wsnet
