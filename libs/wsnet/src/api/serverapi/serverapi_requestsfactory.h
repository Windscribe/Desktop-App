#pragma once
#include "../baserequest.h"
#include "utils/persistentsettings.h"
#include "connectstate.h"
#include "WSNetAdvancedParameters.h"

namespace wsnet {

namespace serverapi_requests_factory
{
    BaseRequest *login(const std::string &username, const std::string &password, const std::string &code2fa,
                       const std::string &sessionTypeId, const std::string &secureToken, const std::string &captchaSolution,
                       const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY, RequestFinishedCallback callback);
    BaseRequest *session(const std::string &authHash, const std::string &appleId, const std::string &gpDeviceId, RequestFinishedCallback callback);
    BaseRequest *claimVoucherCode(const std::string &authHash, const std::string &voucherCode, RequestFinishedCallback callback);
    BaseRequest *deleteSession(const std::string &authHash, RequestFinishedCallback callback);
    BaseRequest *serverLocations(PersistentSettings &persistentSettings, const std::string &language, const std::string &revision,
                                 bool isPro, const std::vector<std::string> &alcList, ConnectState &connectState, WSNetAdvancedParameters *advancedParameters,
                                 RequestFinishedCallback callback);
    BaseRequest *serverCredentials(const std::string &authHash, bool isOpenVpnProtocol, RequestFinishedCallback callback);
    BaseRequest *serverConfigs(const std::string &authHash, const std::string &ovpnVersion, RequestFinishedCallback callback);
    BaseRequest *portMap(const std::string &authHash, std::uint32_t version, const std::vector<std::string> &forceProtocols, RequestFinishedCallback callback);
    BaseRequest *recordInstall(bool isDesktop, const std::string &platform, RequestFinishedCallback callback);

    BaseRequest *addEmail(const std::string &authHash, const std::string &email, RequestFinishedCallback callback);
    BaseRequest *confirmEmail(const std::string &authHash, RequestFinishedCallback callback);
    BaseRequest *signup(const std::string &username, const std::string &password,
                        const std::string &referringUsername, const std::string &email,
                        const std::string &sessionTypeId, const std::string &voucherCode,
                        const std::string &secureToken, const std::string &captchaSolution,
                        const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY,RequestFinishedCallback callback);


    BaseRequest *webSession(const std::string &authHash, RequestFinishedCallback callback);
    BaseRequest *checkUpdate(UpdateChannel updateChannel, const std::string &appVersion, const std::string &appBuild,
                                                          const std::string &osVersion, const std::string &osBuild,
                                                          RequestFinishedCallback callback);
    BaseRequest *debugLog(const std::string &username, const std::string &strLog, RequestFinishedCallback callback);
    BaseRequest *speedRating(const std::string &authHash, const std::string &hostname, const std::string &ip,
                                                         std::int32_t rating, RequestFinishedCallback callback);

    BaseRequest *staticIps(const std::string &authHash, std::uint32_t version, const std::string &platform, const std::string &deviceId, RequestFinishedCallback callback);
    BaseRequest *pingTest(std::uint32_t timeoutMs, RequestFinishedCallback callback);

    BaseRequest *notifications(const std::string &authHash, const std::string &pcpid, const std::string &language, RequestFinishedCallback callback);

    BaseRequest *getRobertFilters(const std::string &authHash, RequestFinishedCallback callback);
    BaseRequest *setRobertFilter(const std::string &authHash, const std::string &id,  std::int32_t status, RequestFinishedCallback callback);
    BaseRequest *syncRobert(const std::string &authHash, RequestFinishedCallback callback);

    BaseRequest *wgConfigsInit(const std::string &authHash, const std::string &clientPublicKey,
                                                           bool deleteOldestKey, RequestFinishedCallback callback);
    BaseRequest *wgConfigsConnect(const std::string &authHash, const std::string &clientPublicKey,
                                                              const std::string &hostname, const std::string &deviceId, const std::string &wgTtl,
                                                              RequestFinishedCallback callback);
    BaseRequest *wgConfigsPskRekey(const std::string &authHash, const std::string &clientPublicKey,
                                                               RequestFinishedCallback callback);

    BaseRequest *myIP(RequestFinishedCallback callback);

    BaseRequest *mobileBillingPlans(const std::string &authHash, const std::string &mobilePlanType, const std::string &promo, int version, RequestFinishedCallback callback);

    BaseRequest *sendPayment(const std::string &authHash, const std::string &appleID, const std::string &appleData, const std::string &appleSIG, RequestFinishedCallback callback);
    BaseRequest *verifyPayment(const std::string &authHash, const std::string &purchaseToken, const std::string &gpPackageName,
                                                           const std::string &gpProductId, const std::string &type,
                                                           const std::string &amazonUserId,
                                                           RequestFinishedCallback callback);

    BaseRequest *postBillingCpid(const std::string &authHash, const std::string &payCpid, RequestFinishedCallback callback);
    BaseRequest *getXpressLoginCode(RequestFinishedCallback callback);
    BaseRequest *verifyXpressLoginCode(const std::string &xpressCode, const std::string &sig, RequestFinishedCallback callback);

    BaseRequest *sendSupportTicket(const std::string &supportEmail, const std::string &supportName,
                                                               const std::string &supportSubject, const std::string &supportMessage,
                                                               const std::string &supportCategory,
                                                               const std::string &type,
                                                               const std::string &channel,
                                                               const std::string &platform,
                                                               RequestFinishedCallback callback);

    BaseRequest *regToken(RequestFinishedCallback callback);
    BaseRequest *signupUsingToken(const std::string &token, RequestFinishedCallback callback);
    BaseRequest *claimAccount(const std::string &authHash, const std::string &username, const std::string &password,
                              const std::string &email, const std::string &voucherCode, const std::string &claimAccount, RequestFinishedCallback callback);

    BaseRequest *shakeData(const std::string &authHash, RequestFinishedCallback callback);
    BaseRequest *recordShakeForDataScore(const std::string &authHash, const std::string &platform,
                                         const std::string &score, const std::string &signature,
                                         RequestFinishedCallback callback);

    BaseRequest *verifyTvLoginCode(const std::string &authHash, const std::string &xpressCode, RequestFinishedCallback callback);
    BaseRequest *cancelAccount(const std::string &authHash, const std::string &password, RequestFinishedCallback callback);
    BaseRequest *sso(const std::string &provider, const std::string &token, RequestFinishedCallback callback);
    BaseRequest *authTokenLogin(bool useAsciiCaptcha, RequestFinishedCallback callback);
    BaseRequest *authTokenSignup(bool useAsciiCaptcha, RequestFinishedCallback callback);
}

} // namespace wsnet
