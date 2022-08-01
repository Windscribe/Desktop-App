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

    d->ss_.set_status(json["status"].toInt());
    d->ss_.set_is_premium(json["is_premium"].toInt() == 1);     // 0 - free, 1 - premium
    d->ss_.set_billing_plan_id(json["billing_plan_id"].toInt());
    d->ss_.set_traffic_used(static_cast<qint64>(json["traffic_used"].toDouble()));
    d->ss_.set_traffic_max(static_cast<qint64>(json["traffic_max"].toDouble()));
    d->ss_.set_user_id(json["user_id"].toString().toStdString());
    d->ss_.set_username(json["username"].toString().toStdString());
    d->ss_.set_email(json["email"].toString().toStdString());
    d->ss_.set_email_status(json["email_status"].toInt());
    d->revisionHash_ = json["loc_hash"].toString();

    if (json.contains("rebill"))
    {
        d->ss_.set_rebill(json["rebill"].toInt());
    }
    if (json.contains("premium_expiry_date"))
    {
        d->ss_.set_premium_expire_date(json["premium_expiry_date"].toString().toStdString());
    }

    d->ss_.clear_alc();
    if (json.contains("alc"))
    {
        const QJsonArray alcArray = json["alc"].toArray();
        for (const QJsonValue &v : alcArray)
        {
            d->ss_.add_alc(v.toString().toStdString());
        }
    }

    d->staticIpsUpdateDevices_.clear();
    if (json.contains("sip"))
    {
        QJsonObject objSip = json["sip"].toObject();
        if (objSip.contains("count"))
        {
            d->ss_.set_static_ips(objSip["count"].toInt());
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

void SessionStatus::initFromProtoBuf(const ProtoTypes::SessionStatus &ss)
{
    d->ss_ = ss;
    d->revisionHash_.clear();
    d->staticIpsUpdateDevices_.clear();
    d->isInitialized_ = true;
}

int SessionStatus::getStaticIpsCount() const
{
    Q_ASSERT(d->isInitialized_);
    return d->ss_.static_ips();
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

bool SessionStatus::isPro() const
{
    Q_ASSERT(d->isInitialized_);
    return d->ss_.is_premium();
}

QStringList SessionStatus::getAlc() const
{
    Q_ASSERT(d->isInitialized_);
    QStringList listAlc;
    for (auto i = 0; i < d->ss_.alc_size(); ++i)
    {
        listAlc << QString::fromStdString(d->ss_.alc(i));
    }
    return listAlc;
}

QString SessionStatus::getUsername() const
{
    Q_ASSERT(d->isInitialized_);
    return QString::fromStdString(d->ss_.username());
}

QString SessionStatus::getUserId() const
{
    Q_ASSERT(d->isInitialized_);
    return QString::fromStdString(d->ss_.user_id());
}

const ProtoTypes::SessionStatus &SessionStatus::getProtoBuf() const
{
    Q_ASSERT(d->isInitialized_);
    return d->ss_;
}

int SessionStatus::getBillingPlanId() const
{
    Q_ASSERT(d->isInitialized_);
    return d->ss_.billing_plan_id();
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
        d->ss_.is_premium() != session.d->ss_.is_premium() ||
        d->ss_.status() != session.d->ss_.status() ||
        d->ss_.rebill() != session.d->ss_.rebill() ||
        d->ss_.billing_plan_id() != session.d->ss_.billing_plan_id() ||
        d->ss_.premium_expire_date() != session.d->ss_.premium_expire_date() ||
        d->ss_.traffic_max() != session.d->ss_.traffic_max() ||
        d->ss_.username() != session.d->ss_.username() ||
        d->ss_.user_id() != session.d->ss_.user_id() ||
        d->ss_.email() != session.d->ss_.email() ||
        d->ss_.email_status() != session.d->ss_.email_status() ||
        d->ss_.static_ips() != session.d->ss_.static_ips() ||
        d->ss_.alc_size() != session.d->ss_.alc_size())
    {
        return  true;
    }
    else
    {
        // compare alc array
        for (int i = 0; i < d->ss_.alc_size(); ++i)
        {
            if (d->ss_.alc(i) != session.d->ss_.alc(i))
            {
                return true;
            }
        }

        return  false;
    }
}

int SessionStatus::getStatus() const
{
    Q_ASSERT(d->isInitialized_);
    return d->ss_.status();
}

} //namespace types
