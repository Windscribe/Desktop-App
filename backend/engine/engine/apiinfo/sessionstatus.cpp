#include "sessionstatus.h"
#include <QJsonArray>

namespace ApiInfo {

SessionStatus::SessionStatus() : isInitialized_(false)
{

}

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

    ss_.set_status(json["status"].toInt());
    ss_.set_is_premium(json["is_premium"].toInt() == 1);     // 0 - free, 1 - premium
    ss_.set_billing_plan_id(json["billing_plan_id"].toInt());
    ss_.set_traffic_used(static_cast<qint64>(json["traffic_used"].toDouble()));
    ss_.set_traffic_max(static_cast<qint64>(json["traffic_max"].toDouble()));
    ss_.set_user_id(json["user_id"].toString().toStdString());
    ss_.set_username(json["username"].toString().toStdString());
    ss_.set_email(json["email"].toString().toStdString());
    ss_.set_email_status(json["email_status"].toInt());
    revisionHash_ = json["loc_hash"].toString();

    if (json.contains("rebill"))
    {
        ss_.set_rebill(json["rebill"].toInt());
    }
    if (json.contains("premium_expiry_date"))
    {
        ss_.set_premium_expire_date(json["premium_expiry_date"].toString().toStdString());
    }

    ss_.clear_alc();
    if (json.contains("alc"))
    {
        QJsonArray alcArray = json["alc"].toArray();
        for (const QJsonValue &v : alcArray)
        {
            ss_.add_alc(v.toString().toStdString());
        }
    }

    staticIpsUpdateDevices_.clear();
    if (json.contains("sip"))
    {
        QJsonObject objSip = json["sip"].toObject();
        if (objSip.contains("count"))
        {
            ss_.set_static_ips(objSip["count"].toInt());
        }
        if (objSip.contains("update"))
        {
            if (objSip["update"].isArray())
            {
                QJsonArray jsonUpdateIps = objSip["update"].toArray();
                for (const QJsonValue &jsonUpdateIpsIt : jsonUpdateIps)
                {
                    staticIpsUpdateDevices_.insert(jsonUpdateIpsIt.toString());
                }
            }
        }
    }

    isInitialized_ = true;
    return true;
}

void SessionStatus::writeToStream(QDataStream &stream) const
{
    Q_ASSERT(isInitialized_);
    size_t size = ss_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    ss_.SerializeToArray(arr.data(), size);
    stream << arr;
}

void SessionStatus::readFromStream(QDataStream &stream)
{
    QByteArray arr;
    stream >> arr;
    ss_.ParseFromArray(arr.data(), arr.size());
    isInitialized_ = true;
}

int SessionStatus::getStaticIpsCount() const
{
    Q_ASSERT(isInitialized_);
    return ss_.static_ips();
}

bool SessionStatus::isContainsStaticDeviceId(const QString &deviceId) const
{
    Q_ASSERT(isInitialized_);
    return staticIpsUpdateDevices_.contains(deviceId);
}

QString SessionStatus::getRevisionHash() const
{
    Q_ASSERT(isInitialized_ && !revisionHash_.isEmpty());
    return revisionHash_;
}

void SessionStatus::setRevisionHash(const QString &revisionHash)
{
    Q_ASSERT(isInitialized_);
    revisionHash_ = revisionHash;
}

bool SessionStatus::isPro() const
{
    Q_ASSERT(isInitialized_);
    return ss_.is_premium();
}

QStringList SessionStatus::getAlc() const
{
    Q_ASSERT(isInitialized_);
    QStringList listAlc;
    for (auto i = 0; i < ss_.alc_size(); ++i)
    {
        listAlc << QString::fromStdString(ss_.alc(i));
    }
    return listAlc;
}

QString SessionStatus::getUsername() const
{
    Q_ASSERT(isInitialized_);
    return QString::fromStdString(ss_.username());
}

const ProtoTypes::SessionStatus &SessionStatus::getProtoBuf() const
{
    Q_ASSERT(isInitialized_);
    return ss_;
}

int SessionStatus::getBillingPlanId() const
{
    Q_ASSERT(isInitialized_);
    return ss_.billing_plan_id();
}

bool SessionStatus::isInitialized() const
{
    return isInitialized_;
}

void SessionStatus::clear()
{
    isInitialized_ = false;
    revisionHash_.clear();
    staticIpsUpdateDevices_.clear();
}

} //namespace ApiInfo
