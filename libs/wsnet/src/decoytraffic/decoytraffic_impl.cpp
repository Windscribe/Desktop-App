#include "decoytraffic_impl.h"
#include <boost/chrono.hpp>
#include <algorithm>
#include "utils/utils.h"
#include "utils/wsnet_logger.h"

namespace wsnet {

DecoyTraffic_impl::DecoyTraffic_impl(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, bool isUpload, std::uint32_t volumePerMinute) :
    io_context_(io_context), httpNetworkManager_(httpNetworkManager),
    isUpload_(isUpload),
    timer_(io_context),
    targetSpeed_(volumePerMinute / 60.0),
    intervalDist_(1.0 / AVERAGE_INTERVAL)
{
    gen_.seed(std::random_device()());
}

DecoyTraffic_impl::~DecoyTraffic_impl()
{
    stop();
}

void DecoyTraffic_impl::setTrafficVolumePerMinute(std::uint32_t volumePerMinute)
{
    std::lock_guard locker(mutex_);
    targetSpeed_ = volumePerMinute / 60.0;
}

void DecoyTraffic_impl::start(std::uint32_t startIntervalSeconds)
{
    std::lock_guard locker(mutex_);
    assert(!bStarted_);

    bStarted_ = true;
    requestFinishedTime_ = {}; // set to zero
    total_ = 0;
    avgNetworkSpeed_ = targetSpeed_;  // initial assumption
    // For the first request we make the size small enough to determine the bandwidth
    // Because if the size is large and the actual bandwidth is small, the request may take a very long time to complete
    averageSize_ = 10000;   // 1 Kb
    sizeDist_ = std::exponential_distribution<>(1.0 / averageSize_);
    speedHistory_.clear();

    timer_.expires_after(std::chrono::seconds(startIntervalSeconds));
    timer_.async_wait(std::bind(&DecoyTraffic_impl::sendJob, this, boost::asio::placeholders::error));
}

double DecoyTraffic_impl::stop()
{
    std::lock_guard locker(mutex_);
    if (!bStarted_) return 0;

    bStarted_ = false;
    if (request_) {
        request_->cancel();
        request_.reset();
    }
    timer_.cancel();
    return total_ / 1000000.0;
}

void DecoyTraffic_impl::sendJob(boost::system::error_code const& err)
{
    std::lock_guard locker(mutex_);
    if (err) return;

     // generate packet size
    initialSize_ = std::max((std::uint32_t)1, (std::uint32_t)(sizeDist_(gen_) + 0.5));
    remainingSize_ = initialSize_;
    requestStartTime_ = std::chrono::high_resolution_clock::now();

    if (isUpload_) {
        sendRequest(remainingSize_, 1);
        //g_logger->info("sendRequest Upload {}", remainingSize_);
    } else {
        sendRequest(1 , remainingSize_);
        //g_logger->info("sendRequest Download {}", remainingSize_);
    }
}

void DecoyTraffic_impl::sendRequest(std::uint32_t dataToSendSize, std::uint32_t dataToReceiveSize)
{
    using namespace std::placeholders;

    if (isUpload_) {
        // 1Mb limit for upload data
        dataToSendSize = std::min(dataToSendSize, (std::uint32_t)1024*1024);
    } else {
        // 6291456 bytes limit for download data (I don't know why, but that's how it's set up on the server side)
        dataToReceiveSize = std::min(dataToReceiveSize, (std::uint32_t)6291456);
    }

    auto postData = "data=" + std::string(dataToSendSize, 'a');
    auto req = httpNetworkManager_->createPostRequest("http://10.255.255.1:8085", 5000, postData);
    req->addHttpHeader("Content-type: text/plain; charset=utf-8");
    req->addHttpHeader("X-DECOY-RESPONSE: " + std::to_string(dataToReceiveSize));
    req->setIsEnableFreshConnect(false);

    //g_logger->info("sendRequest {} {}", dataToSendSize, dataToReceiveSize);

    unsigned int size = isUpload_ ? dataToSendSize : dataToReceiveSize;
    request_ = httpNetworkManager_->executeRequest(req, 1, std::bind(&DecoyTraffic_impl::onFinishedRequest, this, _1, _2, _3, _4, size));
}

void DecoyTraffic_impl::onFinishedRequest(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data,
                                          unsigned int size)
{
    std::lock_guard locker(mutex_);
    if (!error->isSuccess()) {
        //g_logger->warn("Request failed: {}, {}", (int) errCode, curlError);
        // Repeat in a second
        timer_.expires_after(std::chrono::seconds(1));
        timer_.async_wait(std::bind(&DecoyTraffic_impl::sendJob, this,  boost::asio::placeholders::error));
        return;

    } else {
        // always +1 byte for upload/download data
        total_ += size + 1;

        // we send until we send all the data
        if (isUpload_ && size < remainingSize_) {
            remainingSize_ = remainingSize_ - size;
            sendRequest(remainingSize_, 1);
            return;
        } else if (!isUpload_ && data.size() < remainingSize_) {
            remainingSize_ = remainingSize_ - data.size();
            sendRequest(1, remainingSize_);
            return;
        }

        // calculate request execution time
        requestFinishedTime_ = std::chrono::high_resolution_clock::now();
        double sendDuration = std::chrono::duration<double>(requestFinishedTime_ - requestStartTime_).count();

        double currentSpeed = sendDuration > 0 ? initialSize_ / sendDuration : 0;
        updateNetworkSpeed(currentSpeed);

        //g_logger->info("Request finished in {} size {} Mb", sendDuration, initialSize_ / 1024.0f / 1024.0f);

        // adapting sending parameters
        double allowedSpeed = std::min(targetSpeed_, avgNetworkSpeed_);
        // adjusting the packet size to reflect the new interval
        averageSize_ = std::clamp(
            allowedSpeed * AVERAGE_INTERVAL,
            10.0,
            targetSpeed_ * 5.0 // Maximum size 5x of average
            );

        //g_logger->info("averageSize_ {}", averageSize_);

        // distribution update
        sizeDist_ = std::exponential_distribution<>(1.0 / averageSize_);

        // generating the waiting interval
        double delaySeconds = intervalDist_(gen_);
        delaySeconds = std::max(delaySeconds - sendDuration, 0.0);

        //g_logger->info("Delay {}", delaySeconds);

        timer_.expires_after(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<double>(delaySeconds)
            ));
        timer_.async_wait(std::bind(&DecoyTraffic_impl::sendJob, this, boost::asio::placeholders::error));
    }
}

void DecoyTraffic_impl::updateNetworkSpeed(double newSpeed)
{
    speedHistory_.push_back(newSpeed);
    if (speedHistory_.size() > kWindowSize) {
        speedHistory_.pop_front();
    }
    // Calculation of moving average
    avgNetworkSpeed_ = std::accumulate(speedHistory_.begin(), speedHistory_.end(), 0.0) / speedHistory_.size();
}

std::uint32_t DecoyTraffic_impl::toInterval(FakeTrafficType fakeTraffic) const
{
    // for low we create on average large time pauses between requests
    switch (fakeTraffic) {
        case FakeTrafficType::kLow:
            return utils::random(15, 30);
        case FakeTrafficType::kMedium:
            return utils::random(10, 20);
        case FakeTrafficType::kHigh:
            return utils::random(2, 10);
        default:
            assert(false);
            return utils::random(5, 30);
    }
}

} // namespace wsnet
