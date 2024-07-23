#pragma once

#include <queue>
#include <map>
#include <boost/asio.hpp>
#include "WSNetPingManager.h"
#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"
#include "ipingmethod.h"

#ifdef _WIN32
    #include "eventcallbackmanager_win.h"
#elif !defined IS_TVOS
    #include "processmanager.h"
#endif

namespace wsnet {

// Thread safe
class PingManager : public WSNetPingManager
{
public:
    explicit PingManager(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, WSNetAdvancedParameters *advancedParameters);
    virtual ~PingManager();

    std::shared_ptr<WSNetCancelableCallback> ping(const std::string &ip, const std::string &hostname,
                                                  PingType pingType, WSNetPingCallback callback) override;

    void setIsConnectedToVpnState(bool isConnected);

private:
    boost::asio::io_context &io_context_;
    WSNetHttpNetworkManager *httpNetworkManager_;
    WSNetAdvancedParameters *advancedParameters_;

#ifdef _WIN32
    // Required for ICMP pings for Windows system
    EventCallbackManager_win eventCallbackManager_;
#elif !defined IS_TVOS
    // Required for ICMP pings for posix systems
    std::unique_ptr<ProcessManager> processManager_;
#endif
    bool isConnectedToVpn_ = false;
    std::mutex mutex_;
    std::uint64_t curPingId_ = 0;
    std::queue<std::uint64_t> queue_;
    std::map<std::uint64_t, std::unique_ptr<IPingMethod> > map_;

    static constexpr int MAX_PARALLEL_PINGS = 10;
    int curParallelPings_ = 0;


    void onPingMethodFinished(std::uint64_t id);
    IPingMethod *createPingMethod(std::uint64_t id, const std::string &ip, const std::string &hostname, PingType pingType, PingFinishedCallback callback);
    void processNextPingsInQueue();
};

} // namespace wsnet
