#ifndef TYPES_SESSION_STATUS_H
#define TYPES_SESSION_STATUS_H

#include <QString>
#include <QSet>
#include <QJsonObject>
#include <QSharedDataPointer>

namespace types {

class SessionStatusData : public QSharedData
{
public:
    SessionStatusData() : isInitialized_(false) {}

    bool isInitialized_;

    // only this data is serializable
    bool is_premium_;
    qint32 status_; // 2 - disabled
    qint32 rebill_;
    qint32 billing_plan_id_;
    QString premium_expire_date_;
    qint64 traffic_used_;
    qint64 traffic_max_;
    QString username_;
    QString user_id_;
    QString email_;
    qint32 email_status_;
    qint32 static_ips_;
    QStringList alc_; // enabled locations for free users

    // for internal use
    QString revisionHash_;
    QSet<QString> staticIpsUpdateDevices_;
};

// implicitly shared class SessionStatus
class SessionStatus
{
public:
    explicit SessionStatus() : d(new SessionStatusData) {}
    SessionStatus(const SessionStatus &other) : d (other.d) {}

    bool initFromJson(QJsonObject &json, QString &outErrorMessage);

    int getStaticIpsCount() const;
    bool isContainsStaticDeviceId(const QString &deviceId) const;

    QString getRevisionHash() const;
    void setRevisionHash(const QString &revisionHash);
    bool isPremium() const;
    QStringList getAlc() const;
    QString getUsername() const;
    QString getUserId() const;
    QString getEmail() const;
    qint32 getEmailStatus() const;
    qint32 getRebill() const;
    qint32 getBillingPlanId() const;
    QString getPremiumExpireDate() const;
    qint32 getStatus() const;
    qint64 getTrafficUsed() const;
    qint64 getTrafficMax() const;

    bool isInitialized() const;
    void clear();

    bool isChangedForLogging(const SessionStatus &session) const;

    SessionStatus& operator=(const SessionStatus&) = default;

    QString debugString() const;

    friend QDataStream& operator <<(QDataStream& stream, const SessionStatus& ss)
    {
        Q_ASSERT(ss.d->isInitialized_);
        stream << versionForSerialization_;

        stream << ss.d->is_premium_ << ss.d->status_ << ss.d->rebill_ << ss.d->billing_plan_id_ << ss.d->premium_expire_date_ << ss.d->traffic_used_ << ss.d->traffic_max_
               << ss.d->username_ << ss.d->user_id_ << ss.d->email_ << ss.d->email_status_ << ss.d->static_ips_ << ss.d->alc_;

        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, SessionStatus& ss)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version != versionForSerialization_)
        {
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

private:
    QSharedDataPointer<SessionStatusData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types

#endif // TYPES_SESSION_STATUS_H
