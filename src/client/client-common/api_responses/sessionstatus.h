#pragma once

#include <QString>
#include <QSet>
#include <QJsonObject>
#include <QSharedDataPointer>

namespace api_responses {

enum class SessionErrorCode { kSuccess, kSessionInvalid, kBadUsername, kAccountDisabled, kMissingCode2FA, kBadCode2FA, kRateLimited, kUnknownError };


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
    QString last_reset_date_;
    qint64 traffic_used_;
    qint64 traffic_max_;
    QString username_;
    QString user_id_;
    QString email_;
    qint32 email_status_;
    qint32 static_ips_;
    QStringList alc_; // enabled locations for free users

    // for internal use
    SessionErrorCode sessionErrorCode_;
    QString errorMessage_;
    QString authHash_;
    QString revisionHash_;
    QSet<QString> staticIpsUpdateDevices_;
};

// implicitly shared class SessionStatus
class SessionStatus
{
public:
    SessionStatus() : d(new SessionStatusData) {}
    explicit SessionStatus(const std::string &json);
    SessionStatus(const SessionStatus &other) : d (other.d) {}

    int getStaticIpsCount() const;
    bool isContainsStaticDeviceId(const QString &deviceId) const;

    SessionErrorCode getSessionErrorCode() const;
    QString getErrorMessage() const;
    QString getAuthHash() const;
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
    QString getLastResetDate() const;
    qint32 getStatus() const;
    qint64 getTrafficUsed() const;
    qint64 getTrafficMax() const;

    bool isInitialized() const;
    void clear();

    bool isChangedForLogging(const SessionStatus &session) const;

    SessionStatus& operator=(const SessionStatus&) = default;

    QString debugString() const;

    friend QDataStream& operator <<(QDataStream& stream, const SessionStatus& ss);
    friend QDataStream& operator >>(QDataStream& stream, SessionStatus& ss);

private:
    QSharedDataPointer<SessionStatusData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace api_responses
