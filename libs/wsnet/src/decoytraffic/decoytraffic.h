#pragma once

#include <boost/asio.hpp>
#include <deque>
#include "WSNetDecoyTraffic.h"
#include "WSNetHttpNetworkManager.h"
#include "decoytraffic_impl.h"

namespace wsnet {

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
    const double kUploadTrafficShare = 0.6;     // share of upload traffic 60%
    const double kDownloadTrafficShare = 0.4;   // share of download traffic 40%

    DecoyTraffic_impl uploadTraffic_;
    DecoyTraffic_impl downloadTraffic_;
    std::mutex mutex_;
    bool bStarted_ = false;
    std::chrono::time_point<std::chrono::steady_clock> startTime_;

    void stopImpl();

    std::uint32_t toVolumePerMinute(std::uint32_t volume);
};

} // namespace wsnet
