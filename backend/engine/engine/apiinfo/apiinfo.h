#ifndef APIINFO_H
#define APIINFO_H

#include <QDate>
#include <QFile>
#include <QSharedPointer>
#include <QVector>
#include <QSet>
//#include "portmap.h"
//#include "servercredentials.h"
#include "utils/simplecrypt.h"
#include "location.h"
//#include "staticipslocation.h"
#include "sessionstatus.h"

namespace ApiInfo {

class ApiInfo
{
public:
    ApiInfo();
    virtual ~ApiInfo();

    QSharedPointer<SessionStatus> getSessionStatus() const;

    void setSessionStatus(const QSharedPointer<SessionStatus> &value);

    void setLocations(const QVector<QSharedPointer<Location> > &value);
    QVector<QSharedPointer<Location> > getLocations();

    QStringList getForceDisconnectNodes() const;
    void setForceDisconnectNodes(const QStringList &value);

    //void setServerCredentials(const ServerCredentials &serverCredentials);
    //const ServerCredentials &getServerCredentials() const;

    QByteArray getOvpnConfig() const;
    void setOvpnConfig(const QByteArray &value);

    QString getAuthHash() const;
    void setAuthHash(const QString &authHash);

    //QSharedPointer<PortMap> getPortMap() const;
    //void setPortMap(const QSharedPointer<PortMap> &portMap);

    //void setStaticIpsLocation(QSharedPointer<StaticIpsLocation> &value);
    //QSharedPointer<StaticIpsLocation> getStaticIpsLocation() const;

    bool loadFromSettings();
    void saveToSettings();
    static void removeFromSettings();

    //void debugSaveToFile(const QString &filename);
    //void debugLoadFromFile(const QString &filename);

private:
    void processServerLocations();

    QSharedPointer<SessionStatus> sessionStatus_;

    QVector<QSharedPointer<Location> > locations_;

    QStringList forceDisconnectNodes_;

    //ServerCredentials serverCredentials_;

    QByteArray ovpnConfig_;

    //QSharedPointer<PortMap> portMap_;

    //QSharedPointer<StaticIpsLocation> staticIps_;

    QString authHash_;

    SimpleCrypt simpleCrypt_;
};

} //namespace ApiInfo

#endif // APIINFO_H
