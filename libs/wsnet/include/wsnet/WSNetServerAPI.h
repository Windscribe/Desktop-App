#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "scapix_object.h"
#include "WSNetCancelableCallback.h"

namespace wsnet {

enum class UpdateChannel { kRelease = 0, kBeta, kGuineaPig, kInternal };
enum class ApiRetCode { kSuccess = 0, kNetworkError, kNoNetworkConnection, kIncorrectJson, kFailoverFailed, kNoToken };
typedef ApiRetCode ServerApiRetCode; // for backward compatibility
typedef std::function<void(ApiRetCode apiRetCode, const std::string &jsonData)> WSNetRequestFinishedCallback;
typedef std::function<void(std::uint32_t num, std::uint32_t count)> WSNetTryingBackupEndpointCallback;

class WSNetServerAPI : public scapix_object<WSNetServerAPI>
{
public:
    virtual ~WSNetServerAPI() {}

    // Set parameters allowing overriding domains (for example, api-php8.windscribe.com, assets-php8.windscribe.com, checkip-php8.windscribe.com)
    // If the corresponding parameter is empty, the override is not used for the corresponding query type
    // The default behavior - all values are empty
    virtual void setApiResolutionsSettings(const std::string &apiRoot, const std::string &assetsRoot, const std::string &checkIpRoot) = 0;

    virtual void setIgnoreSslErrors(bool bIgnore) = 0;

    // resets the failover state to the initial state
    // useful when you need to force reset from a client
    virtual void resetFailover() = 0;

    // callback function allowing the caller to know which failover is used
    virtual std::shared_ptr<WSNetCancelableCallback> setTryingBackupEndpointCallback(WSNetTryingBackupEndpointCallback tryingBackupEndpointCallback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> login(const std::string &username, const std::string &password, const std::string &code2fa,
                                                           const std::string &secureToken, const std::string &captchaSolution, const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY,
                                                           WSNetRequestFinishedCallback callback) = 0;
    // appleId or gpDeviceId (ios or android) device ID, set them empty if they are not required
    virtual std::shared_ptr<WSNetCancelableCallback> session(const std::string &authHash, const std::string &appleId,
                                                             const std::string &gpDeviceId, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> claimVoucherCode(const std::string &authHash, const std::string &voucherCode, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> deleteSession(const std::string &authHash, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> serverLocations(const std::string &language, const std::string &revision,
                                                                     bool isPro, const std::vector<std::string> &alcList,
                                                                     WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> serverCredentials(const std::string &authHash, bool isOpenVpnProtocol, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> serverConfigs(const std::string &authHash, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> portMap(const std::string &authHash, std::uint32_t version, const std::vector<std::string> &forceProtocols, WSNetRequestFinishedCallback callback) = 0;

    // set isDesktop = true for Desktop clients, or set isDesktop = false for Mobile clients
    virtual std::shared_ptr<WSNetCancelableCallback> recordInstall(bool isDesktop, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> addEmail(const std::string &authHash, const std::string &email, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> confirmEmail(const std::string &authHash, WSNetRequestFinishedCallback callback) = 0;

    // Required: username, password
    // Optionals: referringUsername, email
    virtual std::shared_ptr<WSNetCancelableCallback> signup(const std::string &username, const std::string &password,
                                                            const std::string &referringUsername, const std::string &email,
                                                            const std::string &voucherCode, const std::string &secureToken,
                                                            const std::string &captchaSolution, const std::vector<float> &captchaTrailX,
                                                            const std::vector<float> &captchaTrailY, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> webSession(const std::string &authHash, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> checkUpdate(UpdateChannel updateChannel,
                                                                 const std::string &appVersion, const std::string &appBuild,
                                                                 const std::string &osVersion, const std::string &osBuild,
                                                                 WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> debugLog(const std::string &username, const std::string &strLog, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> speedRating(const std::string &authHash, const std::string &hostname, const std::string &ip,
                                                                 std::int32_t rating, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> staticIps(const std::string &authHash, std::uint32_t version, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> pingTest(std::uint32_t timeoutMs, WSNetRequestFinishedCallback callback) = 0;

    // pcpid parameter is optional and can be empty string
    virtual std::shared_ptr<WSNetCancelableCallback> notifications(const std::string &authHash, const std::string &pcpid, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> getRobertFilters(const std::string &authHash, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> setRobertFilter(const std::string &authHash, const std::string &id,  std::int32_t status, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> syncRobert(const std::string &authHash, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> wgConfigsInit(const std::string &authHash, const std::string &clientPublicKey,
                                                                   bool deleteOldestKey, WSNetRequestFinishedCallback callback) = 0;
    // wgTtl - optional string parameter, for example "3600"
    // this is a wait time before server will discard returned interface address if no handshake is made
    // this helps with device sleep/Idle events where network connectivity be restricted to conserve battery periodically
    virtual std::shared_ptr<WSNetCancelableCallback> wgConfigsConnect(const std::string &authHash, const std::string &clientPublicKey,
                                                                   const std::string &hostname, const std::string &deviceId, const std::string &wgTtl,
                                                                   WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> wgConfigsPskRekey(const std::string &authHash, const std::string &clientPublicKey,
                                                                       WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> myIP(WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> mobileBillingPlans(const std::string &authHash, const std::string &mobilePlanType, const std::string &promo, int version, WSNetRequestFinishedCallback callback) = 0;


    virtual std::shared_ptr<WSNetCancelableCallback> sendPayment(const std::string &authHash, const std::string &appleID, const std::string &appleData, const std::string &appleSIG,
                                                                  WSNetRequestFinishedCallback callback) = 0;

    // Required: purchaseToken
    // Optionals: gpPackageName, gpProductId, type, amazonUserId
    virtual std::shared_ptr<WSNetCancelableCallback> verifyPayment(const std::string &authHash,
                                                                   const std::string &purchaseToken, const std::string &gpPackageName,
                                                                   const std::string &gpProductId, const std::string &type,
                                                                   const std::string &amazonUserId,
                                                                   WSNetRequestFinishedCallback callback) = 0;


    virtual std::shared_ptr<WSNetCancelableCallback> postBillingCpid(const std::string &authHash, const std::string &payCpid, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> getXpressLoginCode(WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> verifyXpressLoginCode(const std::string &xpressCode, const std::string &sig, WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> sendSupportTicket(const std::string &supportEmail, const std::string &supportName,
                                                                       const std::string &supportSubject, const std::string &supportMessage,
                                                                       const std::string &supportCategory,
                                                                       const std::string &type,
                                                                       const std::string &channel,
                                                                       WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> regToken(WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> signupUsingToken(const std::string &token, WSNetRequestFinishedCallback callback) = 0;

    // claimAccount - optional integer but passed as a string. If the string is empty, the parameter is ignored
    virtual std::shared_ptr<WSNetCancelableCallback> claimAccount(const std::string &authHash, const std::string &username, const std::string &password,
                                                                  const std::string &email, const std::string &voucherCode, const std::string &claimAccount,
                                                                  WSNetRequestFinishedCallback callback) = 0;


    virtual std::shared_ptr<WSNetCancelableCallback> shakeData(const std::string &authHash,
                                                                             WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> recordShakeForDataScore(const std::string &authHash,
                                                                             const std::string &score, const std::string &signature,
                                                                             WSNetRequestFinishedCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> verifyTvLoginCode(const std::string &authHash, const std::string &xpressCode,
                                                                       WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> cancelAccount(const std::string &authHash, const std::string &password,
                                                                       WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> sso(const std::string &provider, const std::string &token,
                                                                   WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> authTokenLogin(bool useAsciiCaptcha, WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> authTokenSignup(bool useAsciiCaptcha, WSNetRequestFinishedCallback callback) = 0;
};

} // namespace wsnet
