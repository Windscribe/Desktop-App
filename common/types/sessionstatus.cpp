#include "sessionstatus.h"
#include <QJsonArray>
#include <QMetaType>

const int typeIdSessionStatus = qRegisterMetaType<types::SessionStatus>("types::SessionStatus");

namespace types {

bool SessionStatus::initFromJson(QJsonObject &json, QString &outErrorMessage)
{
    // check for required fields in json
    QStringList required_fields({"status", "is_premium", "billing_plan_id", "traffic_used", "traffic_max", "user_id",
                                 "username", "email", "email_status", "loc_hash"});
    for (const QString &str : required_fields)
    {
        if (!json.contains(str))
        {
            outErrorMessage = str + " field not found";
            return false;
        }
    }

    d->status_ = json["status"].toInt();
    d->is_premium_ = (json["is_premium"].toInt() == 1);  // 0 - free, 1 - premium
    d->billing_plan_id_ = json["billing_plan_id"].toInt();
    d->traffic_used_ = static_cast<qint64>(json["traffic_used"].toDouble());
    d->traffic_max_ = static_cast<qint64>(json["traffic_max"].toDouble());
    d->user_id_ = json["user_id"].toString();
    d->username_ = json["username"].toString();
    d->email_ = json["email"].toString();
    d->email_status_ = json["email_status"].toInt();

    d->revisionHash_ = json["loc_hash"].toString();

    if (json.contains("rebill")) {
        d->rebill_ = json["rebill"].toInt();
    }
    else {
        d->rebill_ = 0;
    }
    if (json.contains("premium_expiry_date")) {
        d->premium_expire_date_ = json["premium_expiry_date"].toString();
    }

    d->alc_.clear();
    if (json.contains("alc"))
    {
        const QJsonArray alcArray = json["alc"].toArray();
        for (const QJsonValue &v : alcArray)
        {
            d->alc_ << v.toString();
        }
    }

    d->staticIpsUpdateDevices_.clear();
    if (json.contains("sip"))
    {
        QJsonObject objSip = json["sip"].toObject();
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

    d->isInitialized_ = true;
    return true;
}

int SessionStatus::getStaticIpsCount() const
{
    Q_ASSERT(d->isInitialized_);
    return d->static_ips_;
}

bool SessionStatus::isContainsStaticDeviceId(const QString &deviceId) const
{
    Q_ASSERT(d->isInitialized_);
    return d->staticIpsUpdateDevices_.contains(deviceId);
}

QString SessionStatus::getRevisionHash() const
{
    Q_ASSERT(d->isInitialized_ && !d->revisionHash_.isEmpty());
    return d->revisionHash_;
}

void SessionStatus::setRevisionHash(const QString &revisionHash)
{
    Q_ASSERT(d->isInitialized_);
    d->revisionHash_ = revisionHash;
}

bool SessionStatus::isPremium() const
{
    Q_ASSERT(d->isInitialized_);
    return d->is_premium_;
}

QStringList SessionStatus::getAlc() const
{
    Q_ASSERT(d->isInitialized_);
    return d->alc_;
}

QString SessionStatus::getUsername() const
{
    Q_ASSERT(d->isInitialized_);
    return d->username_;
}

QString SessionStatus::getUserId() const
{
    Q_ASSERT(d->isInitialized_);
    return d->user_id_;
}

QString SessionStatus::getEmail() const
{
    Q_ASSERT(d->isInitialized_);
    return d->email_;
}

qint32 SessionStatus::getEmailStatus() const
{
    Q_ASSERT(d->isInitialized_);
    return d->email_status_;
}

qint32 SessionStatus::getRebill() const
{
    Q_ASSERT(d->isInitialized_);
    return d->rebill_;
}

qint32 SessionStatus::getBillingPlanId() const
{
    Q_ASSERT(d->isInitialized_);
    return d->billing_plan_id_;
}

QString SessionStatus::getPremiumExpireDate() const
{
    Q_ASSERT(d->isInitialized_);
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
        d->alc_ != session.d->alc_)
    {
        return  true;
    }
    return  false;
}

QString SessionStatus::debugString() const
{
    return QString("[SessionStatus] { is_premium: %1; status: %2; rebill: %3; billing_plan_id: %4; premium_expire_date: %5; traffic_used: %6;"
                   "traffic_max: %7; email_status: %8; static_ips: %9; alc_count: %10 }").arg(d->is_premium_).arg(d->status_).arg(d->rebill_).arg(d->billing_plan_id_)
                    .arg(d->premium_expire_date_).arg(d->traffic_used_).arg(d->traffic_max_).arg(d->email_status_).arg(d->static_ips_).arg(d->alc_.count());
}

qint32 SessionStatus::getStatus() const
{
    Q_ASSERT(d->isInitialized_);
    return d->status_;
}

qint64 SessionStatus::getTrafficUsed() const
{
    Q_ASSERT(d->isInitialized_);
    return d->traffic_used_;
}

qint64 SessionStatus::getTrafficMax() const
{
    Q_ASSERT(d->isInitialized_);
    return d->traffic_max_;
}

QDataStream& operator <<(QDataStream& stream, const SessionStatus& ss)
{
    Q_ASSERT(ss.d->isInitialized_);
    stream << ss.versionForSerialization_;
    stream << ss.d->is_premium_ << ss.d->status_ << ss.d->rebill_ << ss.d->billing_plan_id_ << ss.d->premium_expire_date_ << ss.d->traffic_used_ << ss.d->traffic_max_
           << ss.d->username_ << ss.d->user_id_ << ss.d->email_ << ss.d->email_status_ << ss.d->static_ips_ << ss.d->alc_;
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
           >> ss.d->username_ >> ss.d->user_id_ >> ss.d->email_ >> ss.d->email_status_ >> ss.d->static_ips_ >> ss.d->alc_;
    ss.d->revisionHash_.clear();
    ss.d->staticIpsUpdateDevices_.clear();
    ss.d->isInitialized_ = true;

    return stream;
}

} //namespace types
