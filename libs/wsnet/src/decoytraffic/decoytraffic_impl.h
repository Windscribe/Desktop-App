#pragma once

#include <boost/asio.hpp>
#include <random>
#include <deque>
#include "WSNetHttpNetworkManager.h"

namespace wsnet {

enum class FakeTrafficType { kLow = 0, kMedium, kHigh };

// Thread safe
class DecoyTraffic_impl
{

public:
    explicit DecoyTraffic_impl(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, bool isUpload, std::uint32_t volumePerMinute);
    virtual ~DecoyTraffic_impl();

    void setTrafficVolumePerMinute(std::uint32_t volumePerMinute);
    void start(std::uint32_t startIntervalSeconds);
    // return total upload/download Mbytes for the current session
    double stop();

private:
    std::mutex mutex_;
    boost::asio::io_context &io_context_;
    WSNetHttpNetworkManager *httpNetworkManager_;
    bool isUpload_;
    boost::asio::steady_timer timer_;
    bool bStarted_ = false;
    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;
    std::chrono::time_point<std::chrono::high_resolution_clock> requestFinishedTime_;
    std::chrono::time_point<std::chrono::high_resolution_clock> requestStartTime_;
    std::uint64_t total_;
    std::uint32_t initialSize_;
    std::uint32_t remainingSize_;

    double targetSpeed_;      // target speed (bytes/sec)
    double avgNetworkSpeed_;   // current network speed estimate (bytes/sec)
    double averageSize_;       // current average packet size

    // Average delay between requests is 5 seconds
    static constexpr double AVERAGE_INTERVAL = 5.0;

    std::mt19937 gen_;
    std::exponential_distribution<> intervalDist_;
    std::exponential_distribution<> sizeDist_;

    std::deque<double> speedHistory_; // Velocity history for moving average
    const int kWindowSize = 5;   // Window size for averaging

    void sendJob(const boost::system::error_code &err);

    void sendRequest(uint32_t dataToSendSize, uint32_t dataToReceiveSize);
    void onFinishedRequest(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data,
                           unsigned int size);

    void updateNetworkSpeed(double newSpeed);

    std::uint32_t toInterval(FakeTrafficType fakeTraffic) const;
};


} // namespace wsnet
