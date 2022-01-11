#ifndef APIINFO_SESSION_STATUS_H
#define APIINFO_SESSION_STATUS_H

#include <QString>
#include <QSet>
#include <QJsonObject>
#include <QSharedDataPointer>
#include "utils/protobuf_includes.h"

namespace apiinfo {

class SessionStatusData : public QSharedData
{
public:
    SessionStatusData() : isInitialized_(false) {}

    SessionStatusData(const SessionStatusData &other)
        : QSharedData(other),
          isInitialized_(other.isInitialized_),
          ss_(other.ss_),
          revisionHash_(other.revisionHash_),
          staticIpsUpdateDevices_(other.staticIpsUpdateDevices_) {}
    ~SessionStatusData() {}

    bool isInitialized_;
    ProtoTypes::SessionStatus ss_;  // only this data is saved in settings
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
    void initFromProtoBuf(const ProtoTypes::SessionStatus &ss);

    int getStaticIpsCount() const;
    bool isContainsStaticDeviceId(const QString &deviceId) const;

    QString getRevisionHash() const;
    void setRevisionHash(const QString &revisionHash);
    bool isPro() const;
    QStringList getAlc() const;
    QString getUsername() const;
    QString getUserId() const;
    const ProtoTypes::SessionStatus &getProtoBuf() const;
    int getBillingPlanId() const;

    bool isInitialized() const;
    void clear();

    bool isChangedForLogging(const SessionStatus &session) const;

    SessionStatus& operator=(const SessionStatus&) = default;

private:
    QSharedDataPointer<SessionStatusData> d;
};

} //namespace apiinfo

#endif // APIINFO_SESSION_STATUS_H
