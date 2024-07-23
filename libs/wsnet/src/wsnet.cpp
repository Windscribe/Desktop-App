#define WIN_EXPORT
#include "WSNet.h"

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include "utils/wsnet_callback_sink.h"
#include "utils/persistentsettings.h"
#include "dnsresolver/dnsresolver_cares.h"
#include "httpnetworkmanager/httpnetworkmanager.h"
#include "settings.h"
#include "failover/failovercontainer.h"
#include "serverapi/serverapi.h"
#include "serverapi/wsnet_utils_impl.h"
#include "apiresourcesmanager/apiresourcesmanager.h"
#include "emergencyconnect/emergencyconnect.h"
#include "pingmanager/pingmanager.h"
#include "advancedparameters.h"
#include "connectstate.h"
#include "../../../client/common/version/windscribe_version.h"

#if defined(__ANDROID__)
    #include <scapix/bridge/java/on_load.h>
#endif

namespace wsnet {

class WSNet_impl : public WSNet
{
public:
    WSNet_impl() : work_(boost::asio::make_work_guard(io_context_))
    {
        thread_ = std::thread([this](){ io_context_.run(); });
    }

    virtual ~WSNet_impl()
    {
        apiResourcesManager_.reset();
        work_.reset();  // Allow all handlers to be allowed to finish normally
        thread_.join();
    }

    bool initializeImpl(const std::string &basePlatform,  const std::string &platformName, const std::string &appVersion, const std::string &deviceId,
                        const std::string &openVpnVersion, bool isUseStagingDomains, const std::string &persistentSettings)
    {
        spdlog::info("wsnet version: {}.{}.{}", WINDSCRIBE_MAJOR_VERSION, WINDSCRIBE_MINOR_VERSION, WINDSCRIBE_BUILD_VERSION);

        dnsResolver_ = std::make_shared<DnsResolver_cares>();
        if (!dnsResolver_->init()) {
            spdlog::critical("Failed to initialize DnsResolver");
            return false;
        }

        httpNetworkManager_ = std::make_shared<HttpNetworkManager>(io_context_, dnsResolver_.get());
        if (!httpNetworkManager_->init()) {
            spdlog::critical("Failed to initialize HttpNetworkManager");
            return false;
        }

        Settings::instance().setUseStaging(isUseStagingDomains);
        spdlog::info("Base platform: {}", basePlatform);
        spdlog::info("Platform name: {}", platformName);
        spdlog::info("Use staging domains: {}", isUseStagingDomains);
        spdlog::info("App version: {}", appVersion);
        spdlog::info("OpenVpn version: {}", openVpnVersion);

        Settings::instance().setPlatformName(platformName);
        Settings::instance().setAppVersion(appVersion);
        Settings::instance().setBasePlatform(platformName);
        Settings::instance().setDeviceId(deviceId);
        Settings::instance().setOpenVersionVersion(openVpnVersion);

        persistentSettings_.reset(new PersistentSettings(persistentSettings));

        failoverContainer_ = std::make_unique<FailoverContainer>(httpNetworkManager_.get());
        advancedParameters_ = std::make_shared<AdvancedParameters>();
        serverAPI_ = std::make_shared<ServerAPI>(io_context_, httpNetworkManager_.get(), failoverContainer_.get(), *persistentSettings_, advancedParameters_.get(), connectState_);
        apiResourcesManager_ = std::make_shared<ApiResourcesManager>(io_context_, serverAPI_.get(), *persistentSettings_, connectState_);
        emergencyConnect_ = std::make_shared<EmergencyConnect>(io_context_, failoverContainer_.get(), dnsResolver_.get());
        pingManager_ = std::make_shared<PingManager>(io_context_, httpNetworkManager_.get(), advancedParameters_.get());
        utils_ = std::make_shared<WSNetUtils_impl>(io_context_, httpNetworkManager_.get(), failoverContainer_.get(), advancedParameters_.get());

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

    std::string currentPersistentSettings() override
    {
        return persistentSettings_->getAsString();
    }

    std::shared_ptr<WSNetDnsResolver> dnsResolver() override { return dnsResolver_; }
    std::shared_ptr<WSNetHttpNetworkManager> httpNetworkManager() override { return httpNetworkManager_; }
    std::shared_ptr<WSNetServerAPI> serverAPI() override { return serverAPI_; }
    std::shared_ptr<WSNetApiResourcesManager> apiResourcersManager() override { return apiResourcesManager_; }
    std::shared_ptr<WSNetEmergencyConnect> emergencyConnect() override { return emergencyConnect_; };
    std::shared_ptr<WSNetPingManager> pingManager() override { return pingManager_; }
    std::shared_ptr<WSNetAdvancedParameters> advancedParameters() override { return advancedParameters_; }
    std::shared_ptr<WSNetUtils> utils() override { return utils_; }

private:
    std::thread thread_;
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;

    ConnectState connectState_;
    std::unique_ptr<PersistentSettings> persistentSettings_;
    std::shared_ptr<DnsResolver_cares> dnsResolver_;
    std::shared_ptr<HttpNetworkManager> httpNetworkManager_;
    std::unique_ptr<FailoverContainer> failoverContainer_;
    std::shared_ptr<WSNetAdvancedParameters> advancedParameters_;
    std::shared_ptr<ServerAPI> serverAPI_;
    std::shared_ptr<WSNetApiResourcesManager> apiResourcesManager_;
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

bool WSNet::initialize(const std::string &basePlatform,  const std::string &platformName, const std::string &appVersion, const std::string &deviceId,
                       const std::string &openVpnVersion,
                       bool isUseStagingDomains, const std::string &persistentSettings)
{
    std::lock_guard locker(g_mutex);
    assert(g_wsNet == nullptr);
    g_wsNet.reset(new WSNet_impl);
    return g_wsNet->initializeImpl(basePlatform, platformName, appVersion, deviceId, openVpnVersion, isUseStagingDomains, persistentSettings);
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
