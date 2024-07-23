#include "requestsfactory.h"
#include "setrobertfilter_request.h"
#include "serverlocations_request.h"
#include "utils/utils.h"

#include <cpp-base64/base64.h>
#include <cpp-base64/base64.cpp>

namespace wsnet {

BaseRequest *requests_factory::login(const std::string &username, const std::string &password, const std::string &code2fa, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["username"] = username;
    extraParams["password"] = password;
    extraParams["2fa_code"] = code2fa;
    extraParams["session_type_id"] = "3";

    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "Session", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::session(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "Session", extraParams, callback);
}

BaseRequest *requests_factory::deleteSession(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    return new BaseRequest(HttpMethod::kDelete, SubdomainType::kApi, RequestPriority::kNormal, "Session", extraParams, callback);
}

BaseRequest *requests_factory::serverLocations(PersistentSettings &persistentSettings, const std::string &language, const std::string &revision, bool isPro, const std::vector<std::string> &alcList,
                                               ConnectState &connectState, WSNetAdvancedParameters *advancedParameters, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    // generate alc parameter
    std::string alcField = utils::join(alcList, ",");
    if (!alcField.empty())
        extraParams["alc"] = alcField;

    std::string strIsPro = isPro ? "1" : "0";
    return new ServerLocationsRequest(RequestPriority::kNormal, "/serverlist/mob-v2/" + strIsPro + "/" + revision, extraParams, persistentSettings,
                                      connectState, advancedParameters, callback);
}

BaseRequest *requests_factory::myIP(RequestFinishedCallback callback)
{
    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "MyIp", std::map<std::string, std::string>(), callback);
}

BaseRequest *requests_factory::serverCredentials(const std::string &authHash, bool isOpenVpnProtocol, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["type"] = isOpenVpnProtocol ? "openvpn" : "ikev2";
    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "ServerCredentials", extraParams, callback);
}

BaseRequest *requests_factory::serverConfigs(const std::string &authHash, const std::string &ovpnVersion, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["ovpn_version"] = ovpnVersion;
    auto request = new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "ServerConfigs", extraParams, callback);
    request->setIgnoreJsonParse();
    return request;
}

BaseRequest *requests_factory::portMap(const std::string &authHash, std::uint32_t version, const std::vector<std::string> &forceProtocols, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["version"] = std::to_string(version);
    for (int i = 0; i < forceProtocols.size(); ++i) {
        std::string fp = "force_protocols[" + std::to_string(i) + "]";
        extraParams[fp] = forceProtocols[i];
    }
    auto request = new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "PortMap", extraParams, callback);
    request->setContentTypeHeader("Content-type: application/x-www-form-urlencoded");
    return request;
}

BaseRequest *requests_factory::recordInstall(const std::string &platform, RequestFinishedCallback callback)
{
    auto name = "RecordInstall/app/" + platform;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, name, std::map<std::string, std::string>(), callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::confirmEmail(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["resend_confirmation"] = "1";
    return new BaseRequest(HttpMethod::kPut, SubdomainType::kApi, RequestPriority::kNormal, "Users", extraParams, callback);
}

BaseRequest *requests_factory::addEmail(const std::string &authHash, const std::string &email, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["email"] = email;
    extraParams["email_forced"] = "1";
    return new BaseRequest(HttpMethod::kPut, SubdomainType::kApi, RequestPriority::kNormal, "Users", extraParams, callback);
}

BaseRequest *requests_factory::signup(const std::string &username, const std::string &password, const std::string &referringUsername, const std::string &email, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_type_id"] = "3";
    extraParams["username"] = username;
    extraParams["password"] = password;
    extraParams["referring_username"] = referringUsername;
    extraParams["email"] = email;
    return new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "Users", extraParams, callback);
}

BaseRequest *requests_factory::webSession(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["temp_session"] = "1";
    extraParams["session_type_id"] = "1";
    return new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "WebSession", extraParams, callback);
}

BaseRequest *requests_factory::checkUpdate(UpdateChannel updateChannel, const std::string &appVersion, const std::string &appBuild,
                                           const std::string &osVersion, const std::string &osBuild, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["version"] = appVersion;
    extraParams["build"] = appBuild;

    if (updateChannel == UpdateChannel::kBeta)
        extraParams["beta"] = "1";
    else if (updateChannel == UpdateChannel::kGuineaPig)
        extraParams["beta"] = "2";
    else if (updateChannel == UpdateChannel::kInternal)
        extraParams["beta"] = "3";

    extraParams["os_version"] = osVersion;
    extraParams["os_build"] = osBuild;

    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "CheckUpdate", extraParams, callback);
}

BaseRequest *requests_factory::debugLog(const std::string &username, const std::string &strLog, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["logfile"] = base64_encode(strLog);
    extraParams["username"] = username;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "Report/applog", extraParams, callback);
    request->setContentTypeHeader("Content-type: application/x-www-form-urlencoded");
    return request;
}

BaseRequest *requests_factory::speedRating(const std::string &authHash, const std::string &hostname, const std::string &ip, std::int32_t rating, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["hostname"] = hostname;
    extraParams["ip"] = ip;
    extraParams["rating"] = std::to_string(rating);

    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "SpeedRating", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::staticIps(const std::string &authHash, const std::string &platform, const std::string &deviceId, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["os"] = platform;
    extraParams["device_id"] = deviceId;
    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "StaticIps", extraParams, callback);
}

BaseRequest *requests_factory::pingTest(std::uint32_t timeoutMs, RequestFinishedCallback callback)
{
    auto request = new BaseRequest(HttpMethod::kGet, SubdomainType::kTunnelTest, RequestPriority::kHigh, "PingTest", std::map<std::string, std::string>(), callback);
    request->setIgnoreJsonParse();
    request->setTimeout(timeoutMs);
    request->setUseDnsCache(false);
    return request;
}

BaseRequest *requests_factory::notifications(const std::string &authHash, const std::string &pcpid, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["pcpid"] = pcpid;
    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "Notifications", extraParams, callback);
}

BaseRequest *requests_factory::getRobertFilters(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    return new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "Robert/filters", extraParams, callback);
}

BaseRequest *requests_factory::setRobertFilter(const std::string &authHash, const std::string &id, std::int32_t status, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    std::string json = "{\"filter\":\"" + id + "\", \"status\":" + std::to_string(status) + "}";
    auto request = new SetRobertFilterRequest(HttpMethod::kPut, SubdomainType::kApi, RequestPriority::kNormal, "Robert/filter", extraParams, json, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::syncRobert(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "Robert/syncrobert", extraParams, callback);
    return request;
}

BaseRequest *requests_factory::wgConfigsInit(const std::string &authHash, const std::string &clientPublicKey, bool deleteOldestKey, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["wg_pubkey"] = clientPublicKey;
    if (deleteOldestKey)
        extraParams["force_init"] = "1";

    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kHigh, "WgConfigs/init", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::wgConfigsConnect(const std::string &authHash, const std::string &clientPublicKey, const std::string &hostname, const std::string &deviceId, const std::string &wgTtl, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["wg_pubkey"] = clientPublicKey;
    extraParams["hostname"] = hostname;
    extraParams["device_id"] = deviceId;
    extraParams["wg_ttl"] = wgTtl;

    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kHigh, "WgConfigs/connect", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::mobileBillingPlans(const std::string &authHash, const std::string &mobilePlanType, const std::string &promo, int version, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["mobile_plan_type"] = mobilePlanType;
    extraParams["promo"] = promo;
    extraParams["version"] = std::to_string(version);
    auto request = new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "MobileBillingPlans", extraParams, callback);
    return request;
}

BaseRequest *requests_factory::verifyPayment(const std::string &authHash, const std::string &purchaseToken, const std::string &gpPackageName, const std::string &gpProductId, const std::string &type, const std::string &amazonUserId, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["purchase_token"] = purchaseToken;
    extraParams["gp_package_name"] = gpPackageName;
    extraParams["gp_product_id"] = gpProductId;
    extraParams["type"] = type;
    extraParams["amazon_user_id"] = amazonUserId;

    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "AndroidIPN", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::postBillingCpid(const std::string &authHash, const std::string &payCpid, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["pay_cpid"] = payCpid;

    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "BillingCpid", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::getXpressLoginCode(RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "XpressLogin", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::verifyXpressLoginCode(const std::string &xpressCode, const std::string &sig, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_type_id"] = "3";
    extraParams["xpress_code"] = xpressCode;
    extraParams["sig"] = sig;
    auto request = new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "XpressLogin", extraParams, callback);
    return request;
}

BaseRequest *requests_factory::sendSupportTicket(const std::string &supportEmail, const std::string &supportName, const std::string &supportSubject, const std::string &supportMessage,
                                                 const std::string &supportCategory, const std::string &type, const std::string &channel, const std::string &platform, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["support_email"] = supportEmail;
    extraParams["support_name"] = supportName;
    extraParams["support_subject"] = supportSubject;
    extraParams["support_message"] = supportMessage;
    extraParams["support_category"] = supportCategory;
    extraParams["issue_metadata[type]"] = type;
    extraParams["issue_metadata[channel]"] = channel;
    extraParams["issue_metadata[platform]"] = platform;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "SupportTicket", extraParams, callback);
    request->setContentTypeHeader("Content-type: application/x-www-form-urlencoded");
    return request;
}

BaseRequest *requests_factory::regToken(RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "RegToken", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::signupUsingToken(const std::string &token, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["token"] = token;
    extraParams["session_type_id"] = "4";
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "Users", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::claimAccount(const std::string &authHash, const std::string &username, const std::string &password, const std::string &email, const std::string &claimAccount, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["username"] = username;
    extraParams["password"] = password;
    extraParams["email"] = email;
    extraParams["claim_account"] = claimAccount;
    auto request = new BaseRequest(HttpMethod::kPut, SubdomainType::kApi, RequestPriority::kNormal, "Users", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::sendPayment(const std::string &authHash, const std::string &appleID, const std::string &appleData, const std::string &appleSIG, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["apple_id"] = appleID;
    extraParams["apple_data"] = appleData;
    extraParams["apple_sig"] = appleSIG;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "AppleIPN", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::recordShakeForDataScore(const std::string &authHash, const std::string &platform, const std::string &score, const std::string &signature, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["platform"] = platform;
    extraParams["score"] = score;
    extraParams["sig"] = signature;
    auto request = new BaseRequest(HttpMethod::kPost, SubdomainType::kApi, RequestPriority::kNormal, "ShakeData", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *requests_factory::shakeData(const std::string &authHash, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    auto request = new BaseRequest(HttpMethod::kGet, SubdomainType::kApi, RequestPriority::kNormal, "ShakeData", extraParams, callback);
    return request;
}

BaseRequest *requests_factory::verifyTvLoginCode(const std::string &authHash, const std::string &xpressCode, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["session_auth_hash"] = authHash;
    extraParams["xpress_code"] = xpressCode;
    auto request = new BaseRequest(HttpMethod::kPut, SubdomainType::kApi, RequestPriority::kNormal, "XpressLogin", extraParams, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

} // namespace wsnet
