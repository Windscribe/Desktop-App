#ifndef SESSION_STATUS_H
#define SESSION_STATUS_H

#include <QString>
#include <QSet>
#include <QJsonObject>
#include "ipc/generated_proto/types.pb.h"

class SessionStatus
{
public:
    explicit SessionStatus();

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
    bool isInitialized_;

    ProtoTypes::SessionStatus ss_;  // only this data is saved in settings
    // for internal use
    QString revisionHash_;
    QSet<QString> staticIpsUpdateDevices_;
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

#endif // SESSION_STATUS_H
