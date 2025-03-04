#define WIN_EXPORT
#include "WSNet.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <boost/asio.hpp>
#include "utils/wsnet_callback_sink.h"
#include "utils/persistentsettings.h"
#include "utils/spdlog_utils.h"
#include "utils/wsnet_logger.h"
#include "dnsresolver/dnsresolver_cares.h"
#include "httpnetworkmanager/httpnetworkmanager.h"
#include "settings.h"
#include "failover/failovercontainer.h"
#include "serverapi/serverapi.h"
#include "serverapi/wsnet_utils_impl.h"
#include "apiresourcesmanager/apiresourcesmanager.h"
#include "emergencyconnect/emergencyconnect.h"
#include "pingmanager/pingmanager.h"
#include "decoytraffic/decoytraffic.h"
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
        // no logging by default
        if (!g_logger) {
            g_logger = spdlog::null_logger_mt("wsnet");
        }
        thread_ = std::thread([this](){ io_context_.run(); });
    }

    virtual ~WSNet_impl()
    {
        apiResourcesManager_.reset();
        // This will cause the io_context run() call to return as soon as possible, abandoning unfinished operations and without permitting ready handlers to be dispatched.
        g_logger->info("wsnet io_context_.stop");
        io_context_.stop();
        g_logger->info("wsnet thread_.join");
        thread_.join();
    }

    bool initializeImpl(const std::string &basePlatform,  const std::string &platformName, const std::string &appVersion, const std::string &deviceId,
                        const std::string &openVpnVersion, const std::string &sessionTypeId,
                        bool isUseStagingDomains, const std::string &language, const std::string &persistentSettings)
    {
        g_logger->info("wsnet version: {}.{}.{}", WINDSCRIBE_MAJOR_VERSION, WINDSCRIBE_MINOR_VERSION, WINDSCRIBE_BUILD_VERSION);

        dnsResolver_ = std::make_shared<DnsResolver_cares>();
        if (!dnsResolver_->init()) {
            g_logger->error("Failed to initialize DnsResolver");
            return false;
        }

        httpNetworkManager_ = std::make_shared<HttpNetworkManager>(io_context_, dnsResolver_.get());
        if (!httpNetworkManager_->init()) {
            g_logger->error("Failed to initialize HttpNetworkManager");
            return false;
        }

        Settings::instance().setUseStaging(isUseStagingDomains);
        g_logger->info("Base platform: {}", basePlatform);
        g_logger->info("Platform name: {}", platformName);
        g_logger->info("Use staging domains: {}", isUseStagingDomains);
        g_logger->info("App version: {}", appVersion);
        g_logger->info("OpenVpn version: {}", openVpnVersion);
        g_logger->info("Language: {}", language);

        Settings::instance().setPlatformName(platformName);
        Settings::instance().setAppVersion(appVersion);
        Settings::instance().setBasePlatform(basePlatform);
        Settings::instance().setDeviceId(deviceId);
        Settings::instance().setOpenVpnVersion(openVpnVersion);
        Settings::instance().setLanguage(language);
        Settings::instance().setSessionTypeId(sessionTypeId);

        persistentSettings_.reset(new PersistentSettings(persistentSettings));

        failoverContainer_ = std::make_unique<FailoverContainer>(httpNetworkManager_.get());
        advancedParameters_ = std::make_shared<AdvancedParameters>();
        serverAPI_ = std::make_shared<ServerAPI>(io_context_, httpNetworkManager_.get(), failoverContainer_.get(), *persistentSettings_, advancedParameters_.get(), connectState_);
        apiResourcesManager_ = std::make_shared<ApiResourcesManager>(io_context_, serverAPI_.get(), *persistentSettings_, connectState_);
        emergencyConnect_ = std::make_shared<EmergencyConnect>(io_context_, failoverContainer_.get(), dnsResolver_.get());
        pingManager_ = std::make_shared<PingManager>(io_context_, httpNetworkManager_.get(), advancedParameters_.get(), connectState_);
        decoyTraffic_ = std::make_shared<DecoyTraffic>(io_context_, httpNetworkManager_.get());
        utils_ = std::make_shared<WSNetUtils_impl>(io_context_, httpNetworkManager_.get(), failoverContainer_.get(), advancedParameters_.get());

        return true;
    }

    void setConnectivityState(bool isOnline) override
    {
        connectState_.setConnectivityState(isOnline);
    }
    void setIsConnectedToVpnState(bool isConnected) override
    {
        if (connectState_.isVPNConnected() != isConnected) {
            connectState_.setIsConnectedToVpnState(isConnected);
            // When connecting/disconnecting the VPN clear the DNS cache.
            httpNetworkManager_->clearDnsCache();
        }
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
    std::shared_ptr<WSNetDecoyTraffic> decoyTraffic() override { return decoyTraffic_; }
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
    std::shared_ptr<DecoyTraffic> decoyTraffic_;
    std::shared_ptr<WSNetUtils_impl> utils_;
};

// static methods of WSNet implementation
std::shared_ptr<WSNet_impl> g_wsNet;
std::mutex g_mutex;

void WSNet::setLogger(WSNetLoggerFunction loggerFunction, bool debugLog)
{
    assert(g_logger == nullptr);
    if (loggerFunction) {
        g_logger = callback_logger_mt("wsnet", loggerFunction);
        auto formatter = spdlog_utils::createJsonFormatter();
        g_logger->set_formatter(std::move(formatter));

        if (debugLog)
            g_logger->set_level(spdlog::level::trace);
        else
            g_logger->set_level(spdlog::level::info);
    } else {
        g_logger = spdlog::null_logger_mt("wsnet");
    }
}

bool WSNet::initialize(const std::string &basePlatform,  const std::string &platformName, const std::string &appVersion, const std::string &deviceId,
                       const std::string &openVpnVersion, const std::string &sessionTypeId,
                       bool isUseStagingDomains, const std::string &language, const std::string &persistentSettings)
{
    std::lock_guard locker(g_mutex);
    assert(g_wsNet == nullptr);
    g_wsNet.reset(new WSNet_impl);
    return g_wsNet->initializeImpl(basePlatform, platformName, appVersion, deviceId, openVpnVersion,
                                   sessionTypeId, isUseStagingDomains, language, persistentSettings);
}

std::shared_ptr<WSNet> WSNet::instance()
{
    std::lock_guard locker(g_mutex);
    assert(g_wsNet);
    return g_wsNet;
}

void WSNet::cleanup()
{
    // Added more logs to debug cleanup hangs. Later can be deleted
    g_logger->info("wsnet cleanup started");
    std::lock_guard locker(g_mutex);
    g_wsNet.reset();
    g_logger->info("wsnet cleanup finished");
}

bool WSNet::isValid()
{
    std::lock_guard locker(g_mutex);
    return (g_wsNet != nullptr);
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
