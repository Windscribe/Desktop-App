#include "decoytraffic.h"
#include "utils/wsnet_logger.h"

namespace wsnet {

DecoyTraffic::DecoyTraffic(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager) :
    uploadTraffic_(io_context, httpNetworkManager, true, toVolumePerMinute(0) * kUploadTrafficShare),
    downloadTraffic_(io_context, httpNetworkManager, false, toVolumePerMinute(0) * kDownloadTrafficShare)
{
}

DecoyTraffic::~DecoyTraffic()
{
    stopImpl();
}

void DecoyTraffic::setFakeTrafficVolume(std::uint32_t volume)
{
    std::lock_guard locker(mutex_);
    assert(volume >= 0 && volume <= 2);
    uploadTraffic_.setTrafficVolumePerMinute(toVolumePerMinute(volume) * kUploadTrafficShare);
    downloadTraffic_.setTrafficVolumePerMinute(toVolumePerMinute(volume) * kDownloadTrafficShare);
    g_logger->info("DecoyTraffic setFakeTrafficVolume to {}", volume);
}

void DecoyTraffic::start()
{
    std::lock_guard locker(mutex_);
    if (!bStarted_) {
        bStarted_ = true;
        g_logger->info("Decoy traffic started");
        startTime_ = std::chrono::steady_clock::now();
        // Making slightly different start times for upload and download traffic
        uploadTraffic_.start(1);
        downloadTraffic_.start(3);
    } else {
        return;
    }
}

void DecoyTraffic::stop()
{
    stopImpl();
}

void DecoyTraffic::stopImpl()
{
    std::lock_guard locker(mutex_);
    if (!bStarted_) return;

    bStarted_ = false;
    auto totalUpload = uploadTraffic_.stop();
    auto totalDownload = downloadTraffic_.stop();
    g_logger->info("Decoy traffic stopped. Total time: {} sec. Total upload: {} Mb. Total download: {} Mb.",
                   std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime_).count(),
                   totalUpload, totalDownload);
}

std::uint32_t DecoyTraffic::toVolumePerMinute(std::uint32_t volume)
{
    double bytesPerHour;
    if (volume == 0) {              // low, 1.5 Gb/hour
        bytesPerHour = 1.5 * 1024 * 1024 * 1024;
    } else if (volume == 1) {       // medium, 7 Gb/hour
        bytesPerHour = 7.0 * 1024 * 1024 * 1024;
    } else {                        // high, 18 Gb/hour
        bytesPerHour = 18.0 * 1024 * 1024 * 1024;
    }
    double bytesPerMinute = bytesPerHour / 60.0;
    return bytesPerMinute;
}


} // namespace wsnet
