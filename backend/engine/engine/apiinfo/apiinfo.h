#ifndef APIINFO_H
#define APIINFO_H

#include <QDate>
#include <QFile>
#include <QVector>
#include <QSet>
#include "portmap.h"
#include "servercredentials.h"
#include "utils/simplecrypt.h"
#include "location.h"
#include "staticips.h"
#include "sessionstatus.h"

namespace ApiInfo {

class ApiInfo
{
public:
    explicit ApiInfo();

    SessionStatus getSessionStatus() const;
    void setSessionStatus(const SessionStatus &value);

    void setLocations(const QVector<Location> &value);
    QVector<Location> getLocations() const;

    QStringList getForceDisconnectNodes() const;
    void setForceDisconnectNodes(const QStringList &value);

    void setServerCredentials(const ServerCredentials &serverCredentials);
    ServerCredentials getServerCredentials() const;

    QString getOvpnConfig() const;
    void setOvpnConfig(const QString &value);

    QString getAuthHash() const;
    void setAuthHash(const QString &authHash);

    PortMap getPortMap() const;
    void setPortMap(const PortMap &portMap);

    void setStaticIps(const StaticIps &value);
    StaticIps getStaticIps() const;

    bool loadFromSettings();
    void saveToSettings();
    static void removeFromSettings();

    //void debugSaveToFile(const QString &filename);
    //void debugLoadFromFile(const QString &filename);

private:
    void processServerLocations();

    SessionStatus sessionStatus_;
    QVector<Location> locations_;
    QStringList forceDisconnectNodes_;
    ServerCredentials serverCredentials_;
    QString ovpnConfig_;
    PortMap portMap_;
    StaticIps staticIps_;

    QString authHash_;
    SimpleCrypt simpleCrypt_;

    // for check thread id, access to the class must be from a single thread
    Qt::HANDLE threadId_;
};

} //namespace ApiInfo

#endif // APIINFO_H
