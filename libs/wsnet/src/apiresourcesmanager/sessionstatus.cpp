#include "sessionstatus.h"
#include <rapidjson/document.h>
#include "utils/wsnet_logger.h"

namespace {
    std::string safeGetString(const rapidjson::Document::Object &obj, const std::string &name)
    {
        if (!obj.HasMember(name.c_str()))
            return std::string();
        if (obj[name.c_str()].IsNull())
            return std::string();

        assert(obj[name.c_str()].IsString());
        return obj[name.c_str()].GetString();
    }
}

namespace wsnet {

SessionStatus *SessionStatus::createFromJson(const std::string &json)
{
    if (json.empty()) {
        return nullptr;
    }

    using namespace rapidjson;
    Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        return nullptr;
    }
    SessionStatus *ss = new SessionStatus;
    auto jsonObj = doc.GetObject();

    if (jsonObj.HasMember("errorCode")) {
        int errorCode = jsonObj["errorCode"].GetInt();

        switch (errorCode)
        {
        // 701 - will be returned if the supplied session_auth_hash is invalid. Any authenticated endpoint can
        //       throw this error.  This can happen if the account gets disabled, or they rotate their session
        //       secret (pressed Delete Sessions button in the My Account section).  We should terminate the
        //       tunnel and go to the login screen.
        case 701:
            ss->errorCode_ = SessionErrorCode::kSessionInvalid;
            break;

            // 702 - will be returned ONLY in the login flow, and means the supplied credentials were not valid.
            //       Currently we disregard the API errorMessage and display the hardcoded ones (this is for
            //       multi-language support).
        case 702:
            ss->errorCode_ = SessionErrorCode::kBadUsername;
            break;

            // 703 - deprecated / never returned anymore, however we should still keep this for future purposes.
            //       If 703 is thrown on login (and only on login), display the exact errorMessage to the user,
            //       instead of what we do for 702 errors.
            // 706 - this is thrown only on login flow, and means the target account is disabled or banned.
            //       Do exactly the same thing as for 703 - show the errorMessage.
        case 703:
        case 706:
            ss->errorMessage_ = safeGetString(jsonObj, "errorMessage");
            ss->errorCode_ = SessionErrorCode::kAccountDisabled;
            break;

            // 707 - We have been rate limited by the server.  Ask user to try later.
        case 707:
            ss->errorCode_ = SessionErrorCode::kRateLimited;
            break;

        // 708 - Invalid or expired security token
        case 708:
            ss->errorMessage_ = safeGetString(jsonObj, "errorMessage");
            ss->errorCode_ = SessionErrorCode::kInvalidSecurityToken;
            break;

        case 1340:
            ss->errorCode_ = SessionErrorCode::kMissingCode2FA;
            break;

        case 1341:
            ss->errorCode_ = SessionErrorCode::kBadCode2FA;
            break;

        default:
            ss->errorCode_ = SessionErrorCode::kUnknownError;
        }
        return ss;
    }

    auto data = jsonObj["data"].GetObject();

    // check for required fields in json
    std::vector<std::string> required_fileds = {"status", "is_premium", "billing_plan_id", "traffic_used", "traffic_max", "user_id",
                                                "username", "email", "email_status", "loc_hash"};
    for (const auto &it : required_fileds)
        if (!data.HasMember(it.c_str())) {
            delete ss;
            return nullptr;
        }

    if (data.HasMember("session_auth_hash"))
        ss->authHash_ = safeGetString(data, "session_auth_hash");

    ss->jsonData_ = json;
    ss->status_ = data["status"].GetInt();
    ss->is_premium_ = (data["is_premium"].GetInt() == 1);     // 0 - free, 1 - premium
    ss->billingPlanId_ = data["billing_plan_id"].GetInt();
    ss->trafficUsed_ = data["traffic_used"].GetInt64();
    ss->trafficMax_ = data["traffic_max"].GetInt64();
    ss->userId_ = safeGetString(data, "user_id");
    ss->username_ = safeGetString(data, "username");
    ss->email_ = safeGetString(data, "email");
    ss->emailStatus_ = data["email_status"].GetInt();

    ss->revisionHash_ = safeGetString(data, "loc_hash");

    if (data.HasMember("rebill"))
        ss->rebill_ = data["rebill"].GetInt();
    else
        ss->rebill_ = 0;

    if (data.HasMember("premium_expiry_date"))
        ss->premiumExpireDate_ = safeGetString(data, "premium_expiry_date");

    if (data.HasMember("last_reset")) {
        ss->lastResetDate_ = safeGetString(data, "last_reset");
    }

    ss->alc_.clear();
    if (data.HasMember("alc") && data["alc"].IsArray()) {
        auto alcArray = data["alc"].GetArray();
        for (auto it = alcArray.begin(); it != alcArray.end(); ++it) {
            ss->alc_.push_back(it->GetString());
        }
    }

    ss->staticIps_ = 0;
    ss->staticIpsUpdateDevices_.clear();
    if (data.HasMember("sip") && data["sip"].IsObject()) {
        auto objSip = data["sip"].GetObject();
        if (objSip.HasMember("count")) {
            ss->staticIps_ = objSip["count"].GetInt();
        }
        if (objSip.HasMember("update") && objSip["update"].IsArray()) {
            auto jsonUpdateIps = objSip["update"].GetArray();
            for (auto it = jsonUpdateIps.begin(); it != jsonUpdateIps.end(); ++it) {
                ss->staticIpsUpdateDevices_.insert(it->GetString());
            }
        }
    }

    return ss;
}

std::uint32_t SessionStatus::staticIpsCount() const
{
    return staticIps_;
}

bool SessionStatus::isContainsStaticDeviceId(const std::string &deviceId) const
{
    return staticIpsUpdateDevices_.find(deviceId) != staticIpsUpdateDevices_.end();
}

std::string SessionStatus::revisionHash() const
{
    return revisionHash_;
}

bool SessionStatus::isPremium() const
{
    return is_premium_;
}

std::vector<std::string> SessionStatus::alcList() const
{
    return alc_;
}

std::string SessionStatus::username() const
{
    return username_;
}

std::string SessionStatus::userId() const
{
    return userId_;
}

std::string SessionStatus::email() const
{
    return email_;
}

std::int32_t SessionStatus::emailStatus() const
{
    return emailStatus_;
}

std::int32_t SessionStatus::rebill() const
{
    return rebill_;
}

std::int32_t SessionStatus::billingPlanId() const
{
    return billingPlanId_;
}

std::string SessionStatus::premiumExpiredDate() const
{
    return premiumExpireDate_;
}

std::string SessionStatus::lastResetDate() const
{
    return lastResetDate_;
}

std::int32_t SessionStatus::status() const
{
    return status_;
}

std::int64_t SessionStatus::trafficUsed() const
{
    return trafficUsed_;
}

std::int64_t SessionStatus::trafficMax() const
{
    return trafficMax_;
}

std::string SessionStatus::authHash() const
{
    return authHash_;
}

SessionErrorCode SessionStatus::errorCode() const
{
    return errorCode_;
}

std::string SessionStatus::errorMessage() const
{
    return errorMessage_;
}

std::string SessionStatus::jsonData() const
{
    return jsonData_;
}

void SessionStatus::debugLog() const
{
    g_logger->info("[SessionStatus] (is_premium: {}; status: {}; rebill: {}; billing_plan_id: {}; premium_expire_date: {}; traffic_used: {}; "
                 "traffic_max: {}; email_status: {}; static_ips_count: {}; alc_count: {}; last_reset_date: {})",
                 is_premium_, status_, rebill_, billingPlanId_, premiumExpireDate_, trafficUsed_, trafficMax_, emailStatus_,
                 staticIps_, alc_.size(), lastResetDate_);
}


} // namespace wsnet
