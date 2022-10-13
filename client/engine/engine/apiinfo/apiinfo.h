#pragma once

#include <QDate>
#include <QFile>
#include <QVector>
#include <QSet>
#include <QMap>
#include "types/portmap.h"
#include "servercredentials.h"
#include "utils/simplecrypt.h"
#include "location.h"
#include "staticips.h"
#include "types/sessionstatus.h"

namespace apiinfo {

// Contains data from the Server API that is minimally necessary for the program to switch from the Login screen to Connect Screen
// It can also read and save all data in settings.
class ApiInfo
{
public:
    explicit ApiInfo();

    types::SessionStatus getSessionStatus() const;
    void setSessionStatus(const types::SessionStatus &value);

    void setLocations(const QVector<apiinfo::Location> &value);
    QVector<apiinfo::Location> getLocations() const;

    QStringList getForceDisconnectNodes() const;
    void setForceDisconnectNodes(const QStringList &value);

    void setServerCredentials(const ServerCredentials &serverCredentials);
    ServerCredentials getServerCredentials() const;

    void setServerCredentialsOpenVpn(const QString &username, const QString &password);
    void setServerCredentialsIkev2(const QString &username, const QString &password);
    bool isServerCredentialsOpenVpnInit() const;
    bool isServerCredentialsIkev2Init() const;

    QString getOvpnConfig() const;
    void setOvpnConfig(const QString &value);

    // auth hash is stored in a separate value in QSettings
    static QString getAuthHash();
    static void setAuthHash(const QString &authHash);

    types::PortMap getPortMap() const;
    void setPortMap(const types::PortMap &portMap);

    void setStaticIps(const StaticIps &value);
    StaticIps getStaticIps() const;

    bool loadFromSettings();
    void saveToSettings();
    static void removeFromSettings();

    bool isEverythingInit() const;

private:
    void mergeWindflixLocations();

    types::SessionStatus sessionStatus_;
    QVector<Location> locations_;
    QStringList forceDisconnectNodes_;
    ServerCredentials serverCredentials_;
    QString ovpnConfig_;
    types::PortMap portMap_;
    StaticIps staticIps_;

    bool isSessionStatusInit_ = false;
    bool isLocationsInit_ = false;
    bool isForceDisconnectInit_ = false;
    bool isOvpnConfigInit_ = false;
    bool isPortMapInit_ = false;
    bool isStaticIpsInit_ = false;

    SimpleCrypt simpleCrypt_;

    // for serialization
    static constexpr quint32 magic_ = 0x7605A2AE;
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace apiinfo

