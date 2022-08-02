#ifndef APIINFO_H
#define APIINFO_H

#include <QDate>
#include <QFile>
#include <QVector>
#include <QSet>
#include <QMap>
#include "types/portmap.h"
#include "types/servercredentials.h"
#include "utils/simplecrypt.h"
#include "types/location.h"
#include "types/staticips.h"
#include "types/sessionstatus.h"

namespace apiinfo {

class ApiInfo
{
public:
    explicit ApiInfo();

    types::SessionStatus getSessionStatus() const;
    void setSessionStatus(const types::SessionStatus &value);

    void setLocations(const QVector<types::Location> &value);
    QVector<types::Location> getLocations() const;

    QStringList getForceDisconnectNodes() const;
    void setForceDisconnectNodes(const QStringList &value);

    void setServerCredentials(const types::ServerCredentials &serverCredentials);
    types::ServerCredentials getServerCredentials() const;

    QString getOvpnConfig() const;
    void setOvpnConfig(const QString &value);
    bool ovpnConfigRefetchRequired() const;

    // auth hash is stored in a separate value in QSettings
    static QString getAuthHash();
    static void setAuthHash(const QString &authHash);

    types::PortMap getPortMap() const;
    void setPortMap(const types::PortMap &portMap);

    void setStaticIps(const types::StaticIps &value);
    types::StaticIps getStaticIps() const;

    bool loadFromSettings();
    void saveToSettings();
    static void removeFromSettings();

private:
    void mergeWindflixLocations();

    types::SessionStatus sessionStatus_;
    QVector<types::Location> locations_;
    QStringList forceDisconnectNodes_;
    types::ServerCredentials serverCredentials_;
    QString ovpnConfig_;
    types::PortMap portMap_;
    types::StaticIps staticIps_;

    SimpleCrypt simpleCrypt_;

    // for check thread id, access to the class must be from a single thread
    Qt::HANDLE threadId_;

    QDateTime ovpnConfigSetTimestamp_;

    // for serialization
    static constexpr quint32 magic_ = 0xB4B0C537;
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} //namespace apiinfo

#endif // APIINFO_H
