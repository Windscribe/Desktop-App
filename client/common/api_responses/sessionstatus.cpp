#include "sessionstatus.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaType>
#include "utils/ws_assert.h"

const int typeIdSessionStatus = qRegisterMetaType<api_responses::SessionStatus>("api_responses::SessionStatus");

namespace api_responses {

SessionStatus::SessionStatus(const std::string &json) : d(new SessionStatusData)
{
    d->sessionErrorCode_ = SessionErrorCode::kSuccess;

    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);

    QJsonObject jsonObject = doc.object();

    if (jsonObject.contains("errorCode")) {
        int errorCode = jsonObject["errorCode"].toInt();

        switch (errorCode)
        {
        // 701 - will be returned if the supplied session_auth_hash is invalid. Any authenticated endpoint can
        //       throw this error.  This can happen if the account gets disabled, or they rotate their session
        //       secret (pressed Delete Sessions button in the My Account section).  We should terminate the
        //       tunnel and go to the login screen.
        case 701:
            d->sessionErrorCode_ = SessionErrorCode::kSessionInvalid;
            break;

            // 702 - will be returned ONLY in the login flow, and means the supplied credentials were not valid.
            //       Currently we disregard the API errorMessage and display the hardcoded ones (this is for
            //       multi-language support).
        case 702:
            d->sessionErrorCode_ = SessionErrorCode::kBadUsername;
            break;

            // 703 - deprecated / never returned anymore, however we should still keep this for future purposes.
            //       If 703 is thrown on login (and only on login), display the exact errorMessage to the user,
            //       instead of what we do for 702 errors.
            // 706 - this is thrown only on login flow, and means the target account is disabled or banned.
            //       Do exactly the same thing as for 703 - show the errorMessage.
        case 703:
        case 706:
            d->errorMessage_ = jsonObject["errorMessage"].toString();
            d->sessionErrorCode_ = SessionErrorCode::kAccountDisabled;
            break;

            // 707 - We have been rate limited by the server.  Ask user to try later.
        case 707:
            d->sessionErrorCode_ = SessionErrorCode::kRateLimited;
            break;

        case 1340:
            d->sessionErrorCode_ = SessionErrorCode::kMissingCode2FA;
            break;

        case 1341:
            d->sessionErrorCode_ = SessionErrorCode::kBadCode2FA;
            break;

        default:
            d->sessionErrorCode_ = SessionErrorCode::kUnknownError;
        }

        d->isInitialized_ = true;
        return;
    }


    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (jsonData.contains("session_auth_hash"))
        d->authHash_ = jsonData["session_auth_hash"].toString();

    d->status_ = jsonData["status"].toInt();
    d->is_premium_ = (jsonData["is_premium"].toInt() == 1);  // 0 - free, 1 - premium
    d->billing_plan_id_ = jsonData["billing_plan_id"].toInt();
    d->traffic_used_ = static_cast<qint64>(jsonData["traffic_used"].toDouble());
    d->traffic_max_ = static_cast<qint64>(jsonData["traffic_max"].toDouble());
    d->user_id_ = jsonData["user_id"].toString();
    d->username_ = jsonData["username"].toString();
    d->email_ = jsonData["email"].toString();
    d->email_status_ = jsonData["email_status"].toInt();

    d->revisionHash_ = jsonData["loc_hash"].toString();

    if (jsonData.contains("rebill")) {
        d->rebill_ = jsonData["rebill"].toInt();
    }
    else {
        d->rebill_ = 0;
    }
    if (jsonData.contains("premium_expiry_date")) {
        d->premium_expire_date_ = jsonData["premium_expiry_date"].toString();
    }
    if (jsonData.contains("last_reset")) {
        d->last_reset_date_ = jsonData["last_reset"].toString();
    }

    d->alc_.clear();
    if (jsonData.contains("alc"))
    {
        const QJsonArray alcArray = jsonData["alc"].toArray();
        for (const QJsonValue &v : alcArray)
        {
            d->alc_ << v.toString();
        }
    }

    d->staticIpsUpdateDevices_.clear();
    if (jsonData.contains("sip"))
    {
        QJsonObject objSip = jsonData["sip"].toObject();
        if (objSip.contains("count"))
        {
            d->static_ips_ = objSip["count"].toInt();
        }
        if (objSip.contains("update"))
        {
            if (objSip["update"].isArray())
            {
                const QJsonArray jsonUpdateIps = objSip["update"].toArray();
                for (const QJsonValue &jsonUpdateIpsIt : jsonUpdateIps)
                {
                    d->staticIpsUpdateDevices_.insert(jsonUpdateIpsIt.toString());
                }
            }
        }
    }
    else
    {
        d->static_ips_ = 0;
    }

    d->isInitialized_ = true;
}

int SessionStatus::getStaticIpsCount() const
{
    WS_ASSERT(d->isInitialized_);
    return d->static_ips_;
}

bool SessionStatus::isContainsStaticDeviceId(const QString &deviceId) const
{
    WS_ASSERT(d->isInitialized_);
    return d->staticIpsUpdateDevices_.contains(deviceId);
}

SessionErrorCode SessionStatus::getSessionErrorCode() const
{
    WS_ASSERT(d->isInitialized_);
    return d->sessionErrorCode_;
}

QString SessionStatus::getErrorMessage() const
{
    WS_ASSERT(d->isInitialized_);
    return d->errorMessage_;
}

QString SessionStatus::getAuthHash() const
{
    WS_ASSERT(d->isInitialized_);
    return d->authHash_;
}

QString SessionStatus::getRevisionHash() const
{
    WS_ASSERT(d->isInitialized_ && !d->revisionHash_.isEmpty());
    return d->revisionHash_;
}

void SessionStatus::setRevisionHash(const QString &revisionHash)
{
    WS_ASSERT(d->isInitialized_);
    d->revisionHash_ = revisionHash;
}

bool SessionStatus::isPremium() const
{
    WS_ASSERT(d->isInitialized_);
    return d->is_premium_;
}

QStringList SessionStatus::getAlc() const
{
    WS_ASSERT(d->isInitialized_);
    return d->alc_;
}

QString SessionStatus::getUsername() const
{
    WS_ASSERT(d->isInitialized_);
    return d->username_;
}

QString SessionStatus::getUserId() const
{
    WS_ASSERT(d->isInitialized_);
    return d->user_id_;
}

QString SessionStatus::getEmail() const
{
    WS_ASSERT(d->isInitialized_);
    return d->email_;
}

qint32 SessionStatus::getEmailStatus() const
{
    WS_ASSERT(d->isInitialized_);
    return d->email_status_;
}

qint32 SessionStatus::getRebill() const
{
    WS_ASSERT(d->isInitialized_);
    return d->rebill_;
}

qint32 SessionStatus::getBillingPlanId() const
{
    WS_ASSERT(d->isInitialized_);
    return d->billing_plan_id_;
}

QString SessionStatus::getLastResetDate() const
{
    WS_ASSERT(d->isInitialized_);
    return d->last_reset_date_;
}

QString SessionStatus::getPremiumExpireDate() const
{
    WS_ASSERT(d->isInitialized_);
    return d->premium_expire_date_;
}

bool SessionStatus::isInitialized() const
{
    return d->isInitialized_;
}

void SessionStatus::clear()
{
    d->isInitialized_ = false;
    d->revisionHash_.clear();
    d->staticIpsUpdateDevices_.clear();
}

bool SessionStatus::isChangedForLogging(const SessionStatus &session) const
{
    if (d->isInitialized_ != session.d->isInitialized_ ||
        d->is_premium_ != session.d->is_premium_ ||
        d->status_ != session.d->status_ ||
        d->rebill_ != session.d->rebill_ ||
        d->billing_plan_id_ != session.d->billing_plan_id_ ||
        d->premium_expire_date_ != session.d->premium_expire_date_ ||
        d->traffic_max_ != session.d->traffic_max_ ||
        d->username_ != session.d->username_ ||
        d->user_id_ != session.d->user_id_ ||
        d->email_ != session.d->email_ ||
        d->email_status_ != session.d->email_status_ ||
        d->static_ips_ != session.d->static_ips_ ||
        d->alc_ != session.d->alc_ ||
        d->last_reset_date_ != session.d->last_reset_date_)
    {
        return  true;
    }
    return  false;
}

QString SessionStatus::debugString() const
{
    return QString("[SessionStatus] { is_premium: %1; status: %2; rebill: %3; billing_plan_id: %4; premium_expire_date: %5; traffic_used: %6; "
                   "traffic_max: %7; email_status: %8; static_ips: %9; alc_count: %10; last_reset_date: %11 }").arg(d->is_premium_).arg(d->status_)
                    .arg(d->rebill_).arg(d->billing_plan_id_).arg(d->premium_expire_date_).arg(d->traffic_used_).arg(d->traffic_max_)
                    .arg(d->email_status_).arg(d->static_ips_).arg(d->alc_.count()).arg(d->last_reset_date_);
}

qint32 SessionStatus::getStatus() const
{
    WS_ASSERT(d->isInitialized_);
    return d->status_;
}

qint64 SessionStatus::getTrafficUsed() const
{
    WS_ASSERT(d->isInitialized_);
    return d->traffic_used_;
}

qint64 SessionStatus::getTrafficMax() const
{
    WS_ASSERT(d->isInitialized_);
    return d->traffic_max_;
}

QDataStream& operator <<(QDataStream& stream, const SessionStatus& ss)
{
    WS_ASSERT(ss.d->isInitialized_);
    stream << ss.versionForSerialization_;
    stream << ss.d->is_premium_ << ss.d->status_ << ss.d->rebill_ << ss.d->billing_plan_id_ << ss.d->premium_expire_date_ << ss.d->traffic_used_ << ss.d->traffic_max_
           << ss.d->username_ << ss.d->user_id_ << ss.d->email_ << ss.d->email_status_ << ss.d->static_ips_ << ss.d->alc_ << ss.d->last_reset_date_;
    return stream;
}

QDataStream& operator >>(QDataStream& stream, SessionStatus& ss)
{
    quint32 version;
    stream >> version;
    if (version > ss.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        ss.d->isInitialized_ = false;
        return stream;
    }

    stream >> ss.d->is_premium_ >> ss.d->status_ >> ss.d->rebill_ >> ss.d->billing_plan_id_ >> ss.d->premium_expire_date_ >> ss.d->traffic_used_ >> ss.d->traffic_max_
           >> ss.d->username_ >> ss.d->user_id_ >> ss.d->email_ >> ss.d->email_status_ >> ss.d->static_ips_ >> ss.d->alc_ >> ss.d->last_reset_date_;
    ss.d->revisionHash_.clear();
    ss.d->staticIpsUpdateDevices_.clear();
    ss.d->isInitialized_ = true;

    return stream;
}

} //namespace api_responses
