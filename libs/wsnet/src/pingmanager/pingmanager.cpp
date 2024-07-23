#include "pingmanager.h"
#include <spdlog/spdlog.h>
#include "pingmethod_http.h"

#ifdef _WIN32
    #include "pingmethod_icmp_win.h"
#elif !defined IS_TVOS
    #include "pingmethod_icmp_posix.h"
#endif

namespace wsnet {

PingManager::PingManager(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, WSNetAdvancedParameters *advancedParameters) :
    io_context_(io_context),
    httpNetworkManager_(httpNetworkManager),
    advancedParameters_(advancedParameters)
{

#if !defined _WIN32 && !defined IS_TVOS
    processManager_ = std::make_unique<ProcessManager>(io_context);
#endif
}

PingManager::~PingManager()
{
    std::lock_guard locker(mutex_);
#ifdef _WIN32
    eventCallbackManager_.stop();
#elif !defined IS_TVOS
    processManager_.reset();
#endif
    map_.clear();
}

std::shared_ptr<WSNetCancelableCallback> PingManager::ping(const std::string &ip, const std::string &hostname, PingType pingType, WSNetPingCallback callback)
{
    //TODO: validate ip and hostname?
    std::lock_guard locker(mutex_);

    auto callbackFunc = std::make_shared<CancelableCallback<WSNetPingCallback>>(callback);
    auto ping = createPingMethod(curPingId_, ip, hostname, pingType, callbackFunc);
    map_[curPingId_] = std::unique_ptr<IPingMethod>(ping);

    //TODO: add a delay between pings
    // We do HTTPS-requests right away
    if (pingType == PingType::kHttp) {
        ping->ping(!isConnectedToVpn_);
    }
    // while tcp and icmp are in parallel in queue
    else {
        queue_.push(curPingId_);
        processNextPingsInQueue();
    }
    curPingId_++;
    return callbackFunc;
}

void PingManager::setIsConnectedToVpnState(bool isConnected)
{
    std::lock_guard locker(mutex_);
    isConnectedToVpn_ = isConnected;
}

void PingManager::onPingMethodFinished(std::uint64_t id)
{
    // Executing in thread pool to eliminate deadlocks
    boost::asio::post(io_context_, [this, id] {
        std::lock_guard locker(mutex_);
        auto it = map_.find(id);
        assert(it != map_.end());

        if (it->second->isParallelPing()) {
            curParallelPings_--;
            assert(curParallelPings_ >= 0);
        }

        it->second->callCallback();
        map_.erase(it);
        processNextPingsInQueue();
    });
}

IPingMethod *PingManager::createPingMethod(std::uint64_t id, const std::string &ip, const std::string &hostname, PingType pingType, PingFinishedCallback callback)
{
    if (pingType == PingType::kHttp) {
        return new PingMethodHttp(httpNetworkManager_, id, ip, hostname, false, callback, std::bind(&PingManager::onPingMethodFinished, this, std::placeholders::_1), advancedParameters_);
    } else if (pingType == PingType::kIcmp) {

#ifdef _WIN32
        return new PingMethodIcmp_win(eventCallbackManager_, id, ip, hostname, true, callback, std::bind(&PingManager::onPingMethodFinished, this, std::placeholders::_1));
#elif defined IS_TVOS
    spdlog::error("ICMP pings are not supported on Apple tvOS");
    assert(false);
#else
        return new PingMethodIcmp_posix(id, ip, hostname, true, callback, std::bind(&PingManager::onPingMethodFinished, this, std::placeholders::_1), processManager_.get());
#endif
    } else {
        assert(false);
    }
    return 0;
}

void PingManager::processNextPingsInQueue()
{
    while (curParallelPings_ < MAX_PARALLEL_PINGS && !queue_.empty()) {
        auto id = queue_.front();
        curParallelPings_++;
        auto &ping = map_[id];
        ping->ping(!isConnectedToVpn_);
        queue_.pop();
    }
}

} // namespace wsnet
