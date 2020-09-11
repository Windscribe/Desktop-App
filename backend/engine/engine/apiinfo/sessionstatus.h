#ifndef APIINFO_SESSION_STATUS_H
#define APIINFO_SESSION_STATUS_H

#include <QString>
#include <QSet>
#include <QJsonObject>
#include <QSharedDataPointer>
#include "ipc/generated_proto/types.pb.h"

namespace ApiInfo {

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
    explicit SessionStatus() { d = new SessionStatusData; }
    SessionStatus(const SessionStatus &other) : d (other.d) { }


    bool initFromJson(QJsonObject &json, QString &outErrorMessage);

    void writeToStream(QDataStream &stream) const;
    void readFromStream(QDataStream &stream);

    int getStaticIpsCount() const;
    bool isContainsStaticDeviceId(const QString &deviceId) const;

    QString getRevisionHash() const;
    void setRevisionHash(const QString &revisionHash);
    bool isPro() const;
    QStringList getAlc() const;
    QString getUsername() const;
    const ProtoTypes::SessionStatus &getProtoBuf() const;
    int getBillingPlanId() const;

    bool isInitialized() const;
    void clear();

private:
    QSharedDataPointer<SessionStatusData> d;
};

/*struct SessionStatus
{
    int isPremium;          // 0 - free, 1 - premium
    int status;             // 2 - disabled
    int rebill;
    int billingPlanId;
    QString premiumExpireDateStr;       // "yyyy-MM-dd"
    qint64 trafficUsed;
    qint64 trafficMax;
    QString userId;

    QString username;

    QString email;
    int emailStatus;        // 0 - unconfirmed or 1 - confirmed

    QStringList alc;    // used in serverLocations call for enable some locations for free users
    int staticIps;
    QSet<QString> staticIpsUpdateDevices;

    SessionStatus() : isPremium(0), status(2), rebill(0), billingPlanId(0), trafficUsed(0),
        trafficMax(0), emailStatus(0), staticIps(0) {}

    void writeToStream(QDataStream &stream);
    void readFromStream(QDataStream &stream, int revision);

    bool isPro()
    {
        return isPremium == 1;
    }

    void setRevisionHash(const QString &revisionHash)
    {
        revisionHash_ = revisionHash;
    }

    bool isRevisionHashInitialized() { return !revisionHash_.isEmpty(); }
    QString getRevisionHash()
    {
        Q_ASSERT(!revisionHash_.isEmpty());
        return revisionHash_;
    }

private:
    QString revisionHash_;
};*/

} //namespace ApiInfo

#endif // APIINFO_SESSION_STATUS_H
