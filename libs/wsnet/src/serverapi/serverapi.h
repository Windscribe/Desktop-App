#pragma once

#include "WSNetServerAPI.h"
#include <boost/asio.hpp>
#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"
#include "failover/ifailovercontainer.h"
#include "utils/persistentsettings.h"
#include "connectstate.h"

namespace wsnet {

class ServerAPI_impl;

class ServerAPI : public WSNetServerAPI
{
public:
    explicit ServerAPI(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, IFailoverContainer *failoverContainer,
                       PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState);
    virtual ~ServerAPI();

    void setApiResolutionsSettings(bool isAutomatic, std::string manualAddress) override;
    void setIgnoreSslErrors(bool bIgnore) override;
    std::shared_ptr<WSNetCancelableCallback> setTryingBackupEndpointCallback(WSNetTryingBackupEndpointCallback tryingBackupEndpointCallback) override;

    std::shared_ptr<WSNetCancelableCallback> login(const std::string &username, const std::string &password,
                                                   const std::string &code2fa, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> session(const std::string &authHash, const std::string &appleId,
                                                     const std::string &gpDeviceId, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> claimVoucherCode(const std::string &authHash, const std::string &voucherCode, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> deleteSession(const std::string &authHash, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> serverLocations(const std::string &language, const std::string &revision,
                                                                     bool isPro, const std::vector<std::string> &alcList,
                                                                     WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> serverCredentials(const std::string &authHash, bool isOpenVpnProtocol, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> serverConfigs(const std::string &authHash, WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> portMap(const std::string &authHash, std::uint32_t version, const std::vector<std::string> &forceProtocols, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> recordInstall(bool isDesktop, WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> addEmail(const std::string &authHash, const std::string &email, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> confirmEmail(const std::string &authHash, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> signup(const std::string &username, const std::string &password,
                                                            const std::string &referringUsername, const std::string &email, const std::string &voucherCode,
                                                            WSNetRequestFinishedCallback callback) override;


    std::shared_ptr<WSNetCancelableCallback> webSession(const std::string &authHash, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> checkUpdate(UpdateChannel updateChannel,
                                                         const std::string &appVersion, const std::string &appBuild,
                                                         const std::string &osVersion, const std::string &osBuild,
                                                         WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> debugLog(const std::string &username, const std::string &strLog, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> speedRating(const std::string &authHash, const std::string &hostname, const std::string &ip,
                                                                 std::int32_t rating, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> staticIps(const std::string &authHash, WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> pingTest(std::uint32_t timeoutMs, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> notifications(const std::string &authHash, const std::string &pcpid, WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> getRobertFilters(const std::string &authHash, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> setRobertFilter(const std::string &authHash, const std::string &id,  std::int32_t status, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> syncRobert(const std::string &authHash, WSNetRequestFinishedCallback callback) override;


    std::shared_ptr<WSNetCancelableCallback> wgConfigsInit(const std::string &authHash, const std::string &clientPublicKey,
                                                           bool deleteOldestKey, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> wgConfigsConnect(const std::string &authHash, const std::string &clientPublicKey,
                                                                      const std::string &hostname, const std::string &deviceId, const std::string &wgTtl,
                                                                      WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> myIP(WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> mobileBillingPlans(const std::string &authHash, const std::string &mobilePlanType, const std::string &promo, int version, WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> sendPayment(const std::string &authHash, const std::string &appleID, const std::string &appleData, const std::string &appleSIG,
                                                                 WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> verifyPayment(const std::string &authHash, const std::string &purchaseToken, const std::string &gpPackageName,
                                                           const std::string &gpProductId, const std::string &type, const std::string &amazonUserId,
                                                           WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> postBillingCpid(const std::string &authHash, const std::string &payCpid, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> getXpressLoginCode(WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> verifyXpressLoginCode(const std::string &xpressCode, const std::string &sig, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> sendSupportTicket(const std::string &supportEmail, const std::string &supportName,
                                                                       const std::string &supportSubject, const std::string &supportMessage,
                                                                       const std::string &supportCategory,
                                                                       const std::string &type,
                                                                       const std::string &channel,
                                                                       WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> regToken(WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> signupUsingToken(const std::string &token, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> claimAccount(const std::string &authHash, const std::string &username, const std::string &password,
                                                                  const std::string &email, const std::string &voucherCode, const std::string &claimAccount,
                                                                  WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> shakeData(const std::string &authHash, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> recordShakeForDataScore(const std::string &authHash,
                                                                             const std::string &score, const std::string &signature,
                                                                             WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> verifyTvLoginCode(const std::string &authHash, const std::string &xpressCode,
                                                                       WSNetRequestFinishedCallback callback) override;

    std::shared_ptr<WSNetCancelableCallback> cancelAccount(const std::string &authHash, const std::string &password,
                                                               WSNetRequestFinishedCallback callback) override;

private:
    std::unique_ptr<ServerAPI_impl> impl_;
    boost::asio::io_context &io_context_;
    PersistentSettings &persistentSettings_;
    WSNetAdvancedParameters *advancedParameters_;
    ConnectState &connectState_;
    std::uint32_t subscriberId_;

    void onVPNConnectStateChanged(bool isConnected);
};

} // namespace wsnet
