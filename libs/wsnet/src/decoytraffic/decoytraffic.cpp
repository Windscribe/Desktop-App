#include "decoytraffic.h"
#include <skyr/url.hpp>
#include <boost/chrono.hpp>
#include "utils/utils.h"
#include "utils/wsnet_logger.h"

namespace wsnet {

DecoyTraffic::DecoyTraffic(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager) :
    io_context_(io_context), httpNetworkManager_(httpNetworkManager),
    timer_(io_context)
{
    setFakeTrafficVolume(1);
}

DecoyTraffic::~DecoyTraffic()
{
    stop();
}

void DecoyTraffic::setFakeTrafficVolume(std::uint32_t volume)
{
    std::lock_guard locker(mutex_);
    assert(volume >= 0 && volume <= 2);
    fakeTraffic_ = (FakeTrafficType)volume;
    trafficTrend_.setUpperLimitMultiplier(toMultiplier(fakeTraffic_));
    fakeTrafficVolume_ = toBytes(fakeTraffic_);
    g_logger->info("DecoyTraffic setFakeTrafficVolume to {}", volume);
}

void DecoyTraffic::start()
{
    std::lock_guard locker(mutex_);
    if (!bStarted_) {
        bStarted_ = true;
        g_logger->info("Decoy traffic started");
    } else {
        return;
    }

    lastRequestSendTime_ = {};      // set to zero
    sendTrafficIntervalInSeconds_ = 1;
    startTime_ = std::chrono::steady_clock::now();
    download_ = 0;
    upload_ = 0;
    timer_.expires_after(std::chrono::seconds(sendTrafficIntervalInSeconds_));
    timer_.async_wait(std::bind(&DecoyTraffic::job, this, false, boost::asio::placeholders::error));
}

void DecoyTraffic::stop()
{
    std::lock_guard locker(mutex_);
    if (!bStarted_) return;

    bStarted_ = false;
    g_logger->info("Decoy traffic stopped. Total time: {} sec. Total upload: {}. Total download: {}.",
                   std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime_).count(),
                   upload_, download_);

    if (request_) {
        request_->cancel();
        request_.reset();
    }
    timer_.cancel();
}

void DecoyTraffic::job(bool skipCalc, boost::system::error_code const& err)
{
    std::lock_guard locker(mutex_);
    if (err) return;

    if (!skipCalc) {
        auto timeUsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastRequestSendTime_).count();
        if (timeUsed >= toInterval(fakeTraffic_)) {
            sendTrafficIntervalInSeconds_ = toInterval(fakeTraffic_);
        } else {
            sendTrafficIntervalInSeconds_ = toInterval(fakeTraffic_) - timeUsed;
            if (sendTrafficIntervalInSeconds_ < 1) {
                sendTrafficIntervalInSeconds_ = 1;
            }

            timer_.expires_after(std::chrono::seconds(sendTrafficIntervalInSeconds_));
            timer_.async_wait(std::bind(&DecoyTraffic::job, this, true, boost::asio::placeholders::error));
            return;
        }
    }

    auto dataToSendPerMinute = trafficTrend_.calculateTraffic(fakeTrafficVolume_, true);
    auto dataToSendPerSecond = dataToSendPerMinute / 60 * sendTrafficIntervalInSeconds_;

    auto dataToReceivePerMinute = trafficTrend_.calculateTraffic(fakeTrafficVolume_, false);
    auto dataToReceivePerSecond = dataToReceivePerMinute / 60 * sendTrafficIntervalInSeconds_;

    //g_logger->info("executeRequest {} {}, time diff {}", dataToSendPerSecond, dataToReceivePerSecond,
    //               std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastRequestSendTime_).count());

    lastRequestSendTime_ = std::chrono::steady_clock::now();

    sendRequest(dataToSendPerSecond, dataToReceivePerSecond);
}

void DecoyTraffic::sendRequest(unsigned int dataToSendSize, unsigned int dataToReceiveSize)
{
    using namespace std::placeholders;

    skyr::url_search_parameters sp;
    sp.set("data", std::string(dataToSendSize, 'a'));

    // timeout 100 secs
    auto req = httpNetworkManager_->createPostRequest("http://10.255.255.1:8085", 5000, sp.to_string());
    req->addHttpHeader("Content-type: text/plain; charset=utf-8");
    req->addHttpHeader("X-DECOY-RESPONSE: " + std::to_string(dataToReceiveSize));
    request_ = httpNetworkManager_->executeRequest(req, 1, std::bind(&DecoyTraffic::onFinishedRequest, this, _1, _2, _3, _4, dataToSendSize, dataToReceiveSize));
}

void DecoyTraffic::onFinishedRequest(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data,
                                     unsigned int dataToSendSize, unsigned int dataToReceiveSize)
{
    std::lock_guard locker(mutex_);
    if (!error->isSuccess()) {
        //g_logger->warn("Request failed: {}, {}", (int) errCode, curlError);
    } else {
        //g_logger->warn("Request finished");
        upload_ += dataToSendSize;
        download_ += dataToReceiveSize;
    }
    timer_.expires_after(std::chrono::milliseconds(1));
    timer_.async_wait(std::bind(&DecoyTraffic::job, this, false, boost::asio::placeholders::error));
}

std::uint32_t DecoyTraffic::toBytes(FakeTrafficType fakeTraffic) const
{
    switch (fakeTraffic) {
        case FakeTrafficType::kLow:
            return 100 * 1000;
        case FakeTrafficType::kMedium:
            return 1 * 1000 * 1000;
        case FakeTrafficType::kHigh:
            return 10 * 1000 * 1000;
        default:
            assert(false);
            return 100 * 1000;
    }
}

std::uint32_t DecoyTraffic::toMultiplier(FakeTrafficType fakeTraffic) const
{
    switch (fakeTraffic) {
        case FakeTrafficType::kLow:
            return 175;
        case FakeTrafficType::kMedium:
            return 80;
        case FakeTrafficType::kHigh:
            return 16;
        default:
            assert(false);
            return 175;
    }
}

std::uint32_t DecoyTraffic::toInterval(FakeTrafficType fakeTraffic) const
{
    switch (fakeTraffic) {
        case FakeTrafficType::kLow:
            return utils::random(16, 19);
        case FakeTrafficType::kMedium:
            return utils::random(5, 11);
        case FakeTrafficType::kHigh:
            return utils::random(1, 7);
        default:
            assert(false);
            return utils::random(16, 19);
    }
}

} // namespace wsnet
