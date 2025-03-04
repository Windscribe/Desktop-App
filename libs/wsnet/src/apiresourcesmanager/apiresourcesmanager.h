#pragma once

#include "WSNetApiResourcesManager.h"
#include <boost/asio.hpp>
#include <optional>
#include "WSNetServerAPI.h"
#include "connectstate.h"
#include "sessionstatus.h"
#include "utils/persistentsettings.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

enum class RequestType { kSessionStatus, kLocations, kServerCredentialsOpenVPN, kServerCredentialsIkev2, kServerConfigs, kPortMap, kStaticIps, kNotifications, kCheckUpdate };

class ApiResourcesManager : public WSNetApiResourcesManager
{
public:
    explicit ApiResourcesManager(boost::asio::io_context &io_context, WSNetServerAPI *serverAPI, PersistentSettings &persistentSettings, ConnectState &connectState);
    virtual ~ApiResourcesManager();

    std::shared_ptr<WSNetCancelableCallback> setCallback(WSNetApiResourcesManagerCallback callback) override;

    void setAuthHash(const std::string &authHash) override;

    bool isExist() const override;

    bool loginWithAuthHash() override;
    void login(const std::string &username, const std::string &password, const std::string &code2fa) override;
    void logout() override;

    void fetchSession() override;

    void fetchServerCredentials() override;

    std::string authHash() override;

    // Is this need?
    void removeFromPersistentSettings() override;

    void checkUpdate(UpdateChannel channel, const std::string &appVersion, const std::string &appBuild,
                               const std::string &osVersion, const std::string &osBuild) override;

    void setNotificationPcpid(const std::string &pcpid) override;
    void setMobileDeviceId(const std::string &appleId, const std::string &gpDeviceId) override;

    std::string sessionStatus() const override;
    std::string portMap() const override;
    std::string locations() const override;
    std::string staticIps() const override;
    std::string serverCredentialsOvpn() const override;
    std::string serverCredentialsIkev2() const override;
    std::string serverConfigs() const override;
    std::string notifications() const override;
    std::string checkUpdate() const override;

    void setUpdateIntervals(int sessionInDisconnectedStateMs, int sessionInConnectedStateMs,
                            int locationsMs, int staticIpsMs, int serverConfigsAndCredentialsMs,
                            int portMapMs, int notificationsMs, int checkUpdateMs) override;

private:
    mutable std::mutex mutex_;
    std::shared_ptr<CancelableCallback<WSNetApiResourcesManagerCallback>> callback_ = nullptr;

    boost::asio::io_context &io_context_;
    boost::asio::steady_timer fetchTimer_;
    WSNetServerAPI *serverAPI_;
    PersistentSettings &persistentSettings_;
    ConnectState &connectState_;

    std::unique_ptr<SessionStatus> sessionStatus_;
    std::unique_ptr<SessionStatus> prevSessionStatus_;
    std::string checkUpdate_;

    std::string pcpidNotifications_;

    std::string appleId_;
    std::string gpDeviceId_;

    static constexpr int kMinute = 60 * 1000;
    static constexpr int kHour = 60 * 60 * 1000;
    static constexpr int k24Hours = 24 * 60 * 60 * 1000;

    static constexpr int kDelayBetweenFailedRequests = 1000;

    // update intervals
    int sessionInDisconnectedStateMs_ = kHour;
    int sessionInConnectedStateMs_ = kMinute;
    int locationsMs_ = k24Hours;
    int staticIpsMs_ = k24Hours;
    int serverConfigsAndCredentialsMs_ = k24Hours;
    int portMapMs_ = k24Hours;
    int notificationsMs_ = kHour;
    int checkUpdateMs_ = k24Hours;

    struct UpdateInfo {
        std::chrono::time_point<std::chrono::steady_clock> updateTime;
        bool isRequestSuccess;
    };
    std::map<RequestType, UpdateInfo > lastUpdateTimeMs_;

    std::map<RequestType, std::shared_ptr<wsnet::WSNetCancelableCallback> > requestsInProgress_;

    bool isLoginOkEmitted_ = false;

    // internal variables for fetchServerCredentials() functionality
    bool isFetchingServerCredentials_ = false;
    bool isOpenVpnCredentialsReceived_;
    bool isIkev2CredentialsReceived_;
    bool isServerConfigsReceived_;

    struct
    {
        UpdateChannel channel;
        std::string appVersion;
        std::string appBuild;
        std::string osVersion;
        std::string osBuild;
    } checkUpdateData_;
    bool isCheckUpdateDataSet_ = false;

    void handleLoginOrSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);

    void checkForReadyLogin();
    void checkForServerCredentialsFetchFinished();

    void fetchAll();
    void fetchSession(const std::string &authHash);
    void fetchLocations();
    void fetchStaticIps(const std::string &authHash);
    void fetchServerConfigs(const std::string &authHash);
    void fetchServerCredentialsOpenVpn(const std::string &authHash);
    void fetchServerCredentialsIkev2(const std::string &authHash);
    void fetchPortMap(const std::string &authHash);
    void fetchNotifications(const std::string &authHash);
    void fetchCheckUpdate();

    void updateSessionStatus();

    void onFetchTimer(boost::system::error_code const& err);

    void onInitialSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onLoginAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData,
                       const std::string &username, const std::string &password, const std::string &code2fa);
    void onSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerLocationsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onStaticIpsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerConfigsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerCredentialsOpenVpnAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerCredentialsIkev2Answer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onPortMapAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onNotificationsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onCheckUpdateAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onDeleteSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);

    bool isTimeoutForRequest(RequestType requestType, int timeout);

    void clearValues();
};

} // namespace wsnet
