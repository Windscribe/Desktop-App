#ifndef APIINFO_H
#define APIINFO_H

#include <QDate>
#include <QFile>
#include <QSharedPointer>
#include <QVector>
#include <QSet>
#include "portmap.h"
#include "servercredentials.h"
#include "utils/simplecrypt.h"
#include "serverlocation.h"
#include "staticipslocation.h"

struct SessionStatus
{
    int isPremium;          // 0 - free, 1 - premium
    int status;             // 2 - disabled
    int rebill;
    int billingPlanId;
    QDate premiumExpireDate;
    QString premiumExpireDateStr;
    qint64 trafficUsed;
    qint64 trafficMax;
    QString userId;

    QString username;

    QString email;
    int emailStatus;        // 0 - unconfirmed or 1 - confirmed

    QStringList alc;    // used in serverLocations call for enable some locations for free users
    int staticIps;
    QSet<QString> staticIpsUpdateDevices;

    SessionStatus() : isPremium(0), status(2), rebill(0), trafficUsed(0), trafficMax(0), emailStatus(0), staticIps(0) {}

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
};

// used in Engine class for storage previous session status (need storage only fields isPremium and revisionHash)
class PrevSessionStatus
{
public:
    PrevSessionStatus() : isInitialized_(false) {}

    void set(int isPremium, int billingPlanId, const QString &revisionHash, const QStringList &alcList, int staticIps)
    {
        isPremium_ = isPremium;
        billingPlanId_ = billingPlanId;
        revisionHash_ = revisionHash;
        alcList_ = alcList;
        staticIps_ = staticIps;
        isInitialized_ = true;
    }

    void setRevisionHash(const QString &revisionHash)
    {
        Q_ASSERT(isInitialized_);
        revisionHash_ = revisionHash;
    }

    void clear()
    {
        isInitialized_ = false;
    }

    bool isInitialized() { return isInitialized_; }

    int getIsPremium()
    {
        Q_ASSERT(isInitialized_);
        return isPremium_;
    }

    int getBillingPlanId()
    {
        Q_ASSERT(isInitialized_);
        return billingPlanId_;
    }


    QString getRevisionHash()
    {
        Q_ASSERT(isInitialized_);
        return revisionHash_;
    }

    QStringList getAlcList()
    {
        Q_ASSERT(isInitialized_);
        return alcList_;
    }

    int getStaticIps()
    {
        return staticIps_;
    }

private:
    bool isInitialized_;
    int isPremium_;
    int billingPlanId_;
    QString revisionHash_;
    QStringList alcList_;
    int staticIps_;
};

struct ApiNotification
{
    qint64 id;
    QString title;
    QString message;
    qint64 date;
    int perm_free;
    int perm_pro;
    int popup;
};

struct ApiNotifications
{
    QVector<ApiNotification> notifications;
};

class ApiInfo
{
public:
    ApiInfo();
    virtual ~ApiInfo();

    QSharedPointer<SessionStatus> getSessionStatus() const;

    void setSessionStatus(const QSharedPointer<SessionStatus> &value);

    void setServerLocations(const QVector<QSharedPointer<ServerLocation> > &value);
    QVector<QSharedPointer<ServerLocation> > getServerLocations();

    QStringList getForceDisconnectNodes() const;
    void setForceDisconnectNodes(const QStringList &value);

    void setServerCredentials(const ServerCredentials &serverCredentials);
    const ServerCredentials &getServerCredentials() const;

    QByteArray getOvpnConfig() const;
    void setOvpnConfig(const QByteArray &value);

    QString getAuthHash() const;
    void setAuthHash(const QString &authHash);

    QSharedPointer<PortMap> getPortMap() const;
    void setPortMap(const QSharedPointer<PortMap> &portMap);

    void setStaticIpsLocation(QSharedPointer<StaticIpsLocation> &value);
    QSharedPointer<StaticIpsLocation> getStaticIpsLocation() const;

    bool loadFromSettings();
    void saveToSettings();
    static void removeFromSettings();

    //void debugSaveToFile(const QString &filename);
    //void debugLoadFromFile(const QString &filename);

private:
    void processServerLocations();

    // for check changed formats (for read/write from settings operations)
    // from REVISION_VERSION = 1 to 2 changes:
    // added QStringList proDataCenters to ServerLocation

    // from REVISION_VERSION = 2 to 3 changes:
    // added ikev2 radius username and password

    // from REVISION_VERSION = 3 to 4 changes:
    // added forceExpand flag to ServerLocation

    // from REVISION_VERSION = 4 to 5 changes:
    // added for ServerLocation:
    // bool isBestLocation_, int selectedNodeIndForBestLocation_, int bestLocationPingTimeMs_
    //const int REVISION_VERSION = 6;

    // from REVISION_VERSION = 6 to 7 changes:
    // added field billingPlanId for SessionStatus
    // const int REVISION_VERSION = 7;

    // from REVISION_VERSION = 7 to 8 changes:
    // added field staticIps for SessionStatus
    // added StaticIpsLocation to ApiInfo
    //const int REVISION_VERSION = 8;

    // from REVISION_VERSION = 8 to 9 changes:
    // added field devicename in StaticIpsLocation
    //const int REVISION_VERSION = 9;

    // in revision 10
    // added email and emailStatus in SessionStats
    //const int REVISION_VERSION = 10;

    // in revision 11
    // added premiumExpireDateStr in SessionStats
    const int REVISION_VERSION = 11;

    QSharedPointer<SessionStatus> sessionStatus_;

    QVector<QSharedPointer<ServerLocation> > serverLocations_;

    QStringList forceDisconnectNodes_;

    ServerCredentials serverCredentials_;

    QByteArray ovpnConfig_;

    QSharedPointer<PortMap> portMap_;

    QSharedPointer<StaticIpsLocation> staticIps_;

    QString authHash_;

    SimpleCrypt simpleCrypt_;
};

#endif // APIINFO_H
