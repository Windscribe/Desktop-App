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

    QString getOvpnConfig() const;
    void setOvpnConfig(const QString &value);
    bool ovpnConfigRefetchRequired() const;

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

private:
    void mergeWindflixLocations();

    types::SessionStatus sessionStatus_;
    QVector<Location> locations_;
    QStringList forceDisconnectNodes_;
    ServerCredentials serverCredentials_;
    QString ovpnConfig_;
    types::PortMap portMap_;
    StaticIps staticIps_;

    SimpleCrypt simpleCrypt_;

    // for check thread id, access to the class must be from a single thread
    Qt::HANDLE threadId_;

    QDateTime ovpnConfigSetTimestamp_;

    // for serialization
    static constexpr quint32 magic_ = 0x7605A2AE;
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace apiinfo

