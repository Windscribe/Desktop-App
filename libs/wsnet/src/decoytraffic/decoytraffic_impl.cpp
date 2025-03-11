#include "decoytraffic_impl.h"
#include <boost/chrono.hpp>
#include <algorithm>
#include "utils/utils.h"
#include "utils/wsnet_logger.h"

namespace wsnet {

DecoyTraffic_impl::DecoyTraffic_impl(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, bool isUpload) :
    io_context_(io_context), httpNetworkManager_(httpNetworkManager),
    isUpload_(isUpload),
    timer_(io_context)
{
    setFakeTrafficVolume(0);
}

DecoyTraffic_impl::~DecoyTraffic_impl()
{
    stop();
}

void DecoyTraffic_impl::setFakeTrafficVolume(std::uint32_t volume)
{
    std::lock_guard locker(mutex_);
    assert(volume >= 0 && volume <= 2);
    fakeTraffic_ = (FakeTrafficType)volume;
}

void DecoyTraffic_impl::start(std::uint32_t startIntervalSeconds)
{
    std::lock_guard locker(mutex_);
    assert(!bStarted_);

    bStarted_ = true;
    lastRequestFinishedTime_ = {}; // set to zero
    sendTrafficIntervalInSeconds_ = startIntervalSeconds;
    total_ = 0;
    bandwidth_ = 1000000;   // initial bandwidth estimation 1 Mb/sec
    timer_.expires_after(std::chrono::seconds(sendTrafficIntervalInSeconds_));
    timer_.async_wait(std::bind(&DecoyTraffic_impl::job, this, false, boost::asio::placeholders::error));
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

void DecoyTraffic_impl::job(bool skipCalc, boost::system::error_code const& err)
{
    std::lock_guard locker(mutex_);
    if (err) return;

    if (!skipCalc) {
        auto timeUsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastRequestFinishedTime_).count();
        auto interval = toInterval(fakeTraffic_);
        if (timeUsed >= interval) {
            sendTrafficIntervalInSeconds_ = interval;
        } else {
            sendTrafficIntervalInSeconds_ = interval - timeUsed;
            if (sendTrafficIntervalInSeconds_ < 1) {
                sendTrafficIntervalInSeconds_ = 1;
            }

            timer_.expires_after(std::chrono::seconds(sendTrafficIntervalInSeconds_));
            timer_.async_wait(std::bind(&DecoyTraffic_impl::job, this, true, boost::asio::placeholders::error));
            return;
        }
    }

    // the request is expected to take from 1 to 3 seconds to complete depending on the bandwidth
    // the difference between low, medium, high is adjusted by shortening the time intervals between requests, see toInterval() function
    initialSize_ = bandwidth_ * utils::random(1, 3);
    remainingSize_ = initialSize_;

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
    request_ = httpNetworkManager_->executeRequest(req, 1, std::bind(&DecoyTraffic_impl::onFinishedRequest, this, _1, _2, _3, _4, size, std::chrono::high_resolution_clock::now()));
}

void DecoyTraffic_impl::onFinishedRequest(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data,
                                          unsigned int size, std::chrono::time_point<std::chrono::high_resolution_clock> startTime)
{
    std::lock_guard locker(mutex_);
    if (!error->isSuccess()) {
        //g_logger->warn("Request failed: {}, {}", (int) errCode, curlError);
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

        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsed_seconds = std::chrono::duration<double>(endTime - startTime).count();

        const double alpha = 0.1;  // adaptation factor
        if (elapsed_seconds > 0) {
            double current_bw = initialSize_ / elapsed_seconds;
            bandwidth_ = alpha * current_bw  + (1 - alpha) * bandwidth_;
        }

        lastRequestFinishedTime_ = std::chrono::steady_clock::now();

        //g_logger->info("Request finished in {} size {} Mb", elapsed_seconds, initialSize_ / 1024.0f / 1024.0f);
        //g_logger->info("bandwidth {}", bandwidth_);
    }
    timer_.expires_after(std::chrono::milliseconds(1));
    timer_.async_wait(std::bind(&DecoyTraffic_impl::job, this, false, boost::asio::placeholders::error));
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
