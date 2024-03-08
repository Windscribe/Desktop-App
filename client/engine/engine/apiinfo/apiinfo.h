#pragma once

#include <QDate>
#include <QFile>
#include <QVector>
#include <QSet>
#include <QMap>
#include "servercredentials.h"
#include "utils/simplecrypt.h"
#include "api_responses/staticips.h"
#include "api_responses/sessionstatus.h"
#include "api_responses/location.h"
#include "api_responses/portmap.h"

namespace apiinfo {

// Contains data from the Server API that is minimally necessary for the program to switch from the Login screen to Connect Screen
// It can also read and save all data in settings.
class ApiInfo
{
public:
    explicit ApiInfo();

    api_responses::SessionStatus getSessionStatus() const;
    void setSessionStatus(const api_responses::SessionStatus &value);

    void setLocations(const QVector<api_responses::Location> &value);
    QVector<api_responses::Location> getLocations() const;

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

    api_responses::PortMap getPortMap() const;
    void setPortMap(const api_responses::PortMap &portMap);

    void setStaticIps(const api_responses::StaticIps &value);
    api_responses::StaticIps getStaticIps() const;

    bool loadFromSettings();
    void saveToSettings();
    static void removeFromSettings();

    bool isEverythingInit() const;

    static void clearAutoLoginCredentials();
    static QString autoLoginUsername();
    static QString autoLoginPassword();

private:
    void mergeWindflixLocations();

    // remove all not supported protocols on this OS from portMap_
    void checkPortMapForUnavailableProtocolAndFix();

    api_responses::SessionStatus sessionStatus_;
    QVector<api_responses::Location> locations_;
    QStringList forceDisconnectNodes_;
    ServerCredentials serverCredentials_;
    QString ovpnConfig_;
    api_responses::PortMap portMap_;
    api_responses::StaticIps staticIps_;

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

