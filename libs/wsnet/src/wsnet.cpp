#define WIN_EXPORT
#include "WSNet.h"

#include <spdlog/spdlog.h>
#include <BS_thread_pool.hpp>
#include "utils/wsnet_callback_sink.h"
#include "dnsresolver/dnsresolver_cares.h"
#include "httpnetworkmanager/httpnetworkmanager.h"
#include "settings.h"
#include "failover/failovercontainer.h"
#include "serverapi/serverapi.h"
#include "serverapi/wsnet_utils_impl.h"
#include "emergencyconnect/emergencyconnect.h"
#include "pingmanager/pingmanager.h"
#include "advancedparameters.h"
#include "connectstate.h"
#include "wsnet_version.h"

#if defined(__ANDROID__)
    #include <scapix/bridge/java/on_load.h>
#endif

namespace wsnet {

class WSNet_impl : public WSNet
{
public:
    WSNet_impl() : taskQueue_(1)
    {
    }

    virtual ~WSNet_impl()
    {
        // delete all objects in the task queue for thread safety
        taskQueue_.detach_task([this] {
            utils_.reset();
            pingManager_.reset();
            emergencyConnect_.reset();
            serverAPI_.reset();
            advancedParameters_.reset();
            httpNetworkManager_.reset();
            dnsResolver_.reset();
        });
        taskQueue_.wait();
    }

    bool initializeImpl(const std::string &platformName,const std::string &appVersion, bool isUseStagingDomains, const std::string &serverApiSettings)
    {
        spdlog::info("wsnet version: {}.{}.{}", wsnet_VERSION_MAJOR, wsnet_VERSION_MINOR, wsnet_VERSION_PATCH);

        dnsResolver_ = std::make_shared<DnsResolver_cares>();
        if (!dnsResolver_->init()) {
            spdlog::critical("Failed to initialize DnsResolver");
            return false;
        }

        httpNetworkManager_ = std::make_shared<HttpNetworkManager>(taskQueue_, dnsResolver_.get());
        if (!httpNetworkManager_->init()) {
            spdlog::critical("Failed to initialize HttpNetworkManager");
            return false;
        }

        Settings::instance().setUseStaging(isUseStagingDomains);
        spdlog::info("Use staging domains: {}", isUseStagingDomains);
        Settings::instance().setPlatformName(platformName);
        Settings::instance().setAppVersion(appVersion);
        spdlog::info("Platform name: {}", platformName);
        spdlog::info("App version: {}", appVersion);

        failoverContainer_ = std::make_unique<FailoverContainer>(httpNetworkManager_.get());
        advancedParameters_ = std::make_shared<AdvancedParameters>();
        serverAPI_ = std::make_shared<ServerAPI>(taskQueue_, httpNetworkManager_.get(), failoverContainer_.get(), serverApiSettings, advancedParameters_.get(), connectState_);
        emergencyConnect_ = std::make_shared<EmergencyConnect>(taskQueue_, failoverContainer_.get(), dnsResolver_.get());
        pingManager_ = std::make_shared<PingManager>(taskQueue_, httpNetworkManager_.get());
        utils_ = std::make_shared<WSNetUtils_impl>(taskQueue_, httpNetworkManager_.get(), failoverContainer_.get(), advancedParameters_.get());

        return true;
    }

    void setConnectivityState(bool isOnline) override
    {
        connectState_.setConnectivityState(isOnline);
    }
    void setIsConnectedToVpnState(bool isConnected) override
    {
        connectState_.setIsConnectedToVpnState(isConnected);
    }

    std::shared_ptr<WSNetDnsResolver> dnsResolver() override { return dnsResolver_; }
    std::shared_ptr<WSNetHttpNetworkManager> httpNetworkManager() override { return httpNetworkManager_; }
    std::shared_ptr<WSNetServerAPI> serverAPI() override { return serverAPI_; }
    std::shared_ptr<WSNetEmergencyConnect> emergencyConnect() override { return emergencyConnect_; };
    std::shared_ptr<WSNetPingManager> pingManager() override { return pingManager_; }
    std::shared_ptr<WSNetAdvancedParameters> advancedParameters() override { return advancedParameters_; }
    std::shared_ptr<WSNetUtils> utils() override { return utils_; }

private:
    BS::thread_pool taskQueue_;
    ConnectState connectState_;
    std::shared_ptr<DnsResolver_cares> dnsResolver_;
    std::shared_ptr<HttpNetworkManager> httpNetworkManager_;
    std::unique_ptr<FailoverContainer> failoverContainer_;
    std::shared_ptr<WSNetAdvancedParameters> advancedParameters_;
    std::shared_ptr<ServerAPI> serverAPI_;
    std::shared_ptr<EmergencyConnect> emergencyConnect_;
    std::shared_ptr<PingManager> pingManager_;
    std::shared_ptr<WSNetUtils_impl> utils_;
};

// static methods of WSNet implementation
std::shared_ptr<WSNet_impl> g_wsNet;
std::mutex g_mutex;

void WSNet::setLogger(WSNetLoggerFunction loggerFunction, bool debugLog)
{
    if (loggerFunction) {
        auto logger = callback_logger_mt("wsnet", loggerFunction);
        spdlog::set_default_logger(logger);
        if (debugLog)
            spdlog::set_level(spdlog::level::trace);
        else
            spdlog::set_level(spdlog::level::info);
    } else {
        spdlog::set_level(spdlog::level::off);
        spdlog::shutdown();
    }
}

bool WSNet::initialize(const std::string &platformName, const std::string &appVersion, bool isUseStagingDomains, const std::string &serverApiSettings)
{
    std::lock_guard locker(g_mutex);
    assert(g_wsNet == nullptr);
    g_wsNet.reset(new WSNet_impl);
    return g_wsNet->initializeImpl(platformName, appVersion, isUseStagingDomains, serverApiSettings);
}

std::shared_ptr<WSNet> WSNet::instance()
{
    std::lock_guard locker(g_mutex);
    assert(g_wsNet);
    return g_wsNet;
}

void WSNet::cleanup()
{
    std::lock_guard locker(g_mutex);
    g_wsNet.reset();
}

} // namespace wsnet

#if defined(__ANDROID__)
JavaVM *g_javaVM = nullptr;

// We must set JavaVM in the c-ares library (see: https://c-ares.org/ares_library_init_android.html)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    g_javaVM = vm;
    return scapix_JNI_OnLoad(vm, reserved);
}

#endif
