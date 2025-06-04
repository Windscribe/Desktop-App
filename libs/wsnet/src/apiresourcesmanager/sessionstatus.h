#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <rapidjson/document.h>

namespace wsnet {

enum class SessionErrorCode { kSuccess, kSessionInvalid, kBadUsername, kAccountDisabled, kMissingCode2FA, kBadCode2FA, kRateLimited, kInvalidSecurityToken, kUnknownError };

// wrapper for SessionStatus json text
class SessionStatus
{
public:
    // return nullptr if failed
    static SessionStatus *createFromJson(const std::string &json);

    // copy constructor
    SessionStatus(const SessionStatus *ss) {
        *this = *ss;
    }

    std::uint32_t staticIpsCount() const;
    bool isContainsStaticDeviceId(const std::string &deviceId) const;

    std::string revisionHash() const;
    bool isPremium() const;
    std::vector<std::string> alcList() const;
    std::string username() const;
    std::string userId() const;
    std::string email() const;
    std::int32_t emailStatus() const;
    std::int32_t rebill() const;
    std::int32_t billingPlanId() const;
    std::string premiumExpiredDate() const;
    std::string lastResetDate() const;
    std::int32_t status() const;
    std::int64_t trafficUsed() const;
    std::int64_t trafficMax() const;

    std::string authHash() const;
    SessionErrorCode errorCode() const;
    std::string errorMessage() const;

    std::string jsonData() const;

    void debugLog() const;

private:
    SessionStatus() {}; // can only be created by the static function createFromJson(...)

    std::string jsonData_;
    bool is_premium_;
    std::int32_t status_; // 2 - disabled
    std::int32_t rebill_;
    std::int32_t billingPlanId_;
    std::string premiumExpireDate_;
    std::string lastResetDate_;
    std::int64_t trafficUsed_;
    std::int64_t trafficMax_;
    std::string username_;
    std::string userId_;
    std::string email_;
    std::int32_t emailStatus_;
    std::uint32_t staticIps_;
    std::vector<std::string> alc_; // enabled locations for free users

    SessionErrorCode errorCode_ = SessionErrorCode::kSuccess;
    std::string errorMessage_;
    std::string authHash_;  // can be empty for Session request
    std::string revisionHash_;
    std::set<std::string> staticIpsUpdateDevices_;
};

} // namespace wsnet
