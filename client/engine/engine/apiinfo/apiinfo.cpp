#include "apiinfo.h"
#include <QThread>
#include <QSettings>
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/protobuf_includes.h"

namespace apiinfo {

ApiInfo::ApiInfo() : simpleCrypt_(0x4572A4ACF31A31BA), threadId_(QThread::currentThreadId())
{
}

SessionStatus ApiInfo::getSessionStatus() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return sessionStatus_;
}

void ApiInfo::setSessionStatus(const SessionStatus &value)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    sessionStatus_ = value;
    QSettings settings;
    settings.setValue("userId", sessionStatus_.getUserId());    // need for uninstaller program for open post uninstall webpage
}

void ApiInfo::setLocations(const QVector<Location> &value)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    locations_ = value;
    mergeWindflixLocations();
}

QVector<Location> ApiInfo::getLocations() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return locations_;
}

QStringList ApiInfo::getForceDisconnectNodes() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return forceDisconnectNodes_;
}

void ApiInfo::setForceDisconnectNodes(const QStringList &value)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    forceDisconnectNodes_ = value;
}

void ApiInfo::setServerCredentials(const ServerCredentials &serverCredentials)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    serverCredentials_ = serverCredentials;
}

ServerCredentials ApiInfo::getServerCredentials() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return serverCredentials_;
}

QString ApiInfo::getOvpnConfig() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return ovpnConfig_;
}

void ApiInfo::setOvpnConfig(const QString &value)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    ovpnConfig_ = value;
    ovpnConfigSetTimestamp_ = QDateTime::currentDateTimeUtc();
}

// return empty string if auth hash not exist in the settings
QString ApiInfo::getAuthHash()
{
    QString authHash;
    QSettings settings2;
    authHash = settings2.value("authHash", "").toString();
    if (authHash.isEmpty())
    {
        // try load from settings of the app ver1
        QSettings settings1("Windscribe", "Windscribe");
        authHash = settings1.value("authHash", "").toString();
    }
    return authHash;
}

void ApiInfo::setAuthHash(const QString &authHash)
{
    QSettings settings;
    settings.setValue("authHash", authHash);
}

PortMap ApiInfo::getPortMap() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return portMap_;
}

void ApiInfo::setPortMap(const PortMap &portMap)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    portMap_ = portMap;
}

void ApiInfo::setStaticIps(const StaticIps &value)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    staticIps_ = value;
}

StaticIps ApiInfo::getStaticIps() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return staticIps_;
}

void ApiInfo::saveToSettings()
{
    /*Q_ASSERT(threadId_ == QThread::currentThreadId());

    QSettings settings;
    ProtoApiInfo::ApiInfo protoApiInfo;

    *protoApiInfo.mutable_session_status() = sessionStatus_.getProtoBuf();

    for (const Location &l : locations_)
    {
        *protoApiInfo.add_locations() = l.getProtoBuf();
    }

    *protoApiInfo.mutable_server_credentials() = serverCredentials_.getProtoBuf();
    protoApiInfo.set_ovpn_config(ovpnConfig_.toStdString());
    *protoApiInfo.mutable_port_map() = portMap_.getProtoBuf();
    *protoApiInfo.mutable_static_ips() = staticIps_.getProtoBuf();

    size_t size = protoApiInfo.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    protoApiInfo.SerializeToArray(arr.data(), size);

    settings.setValue("apiInfo", simpleCrypt_.encryptToString(arr));

    if (!sessionStatus_.getRevisionHash().isEmpty())
    {
        settings.setValue("revisionHash", sessionStatus_.getRevisionHash());
    }
    else
    {
        settings.remove("revisionHash");
    }*/
}

void ApiInfo::removeFromSettings()
{
    {
        QSettings settings;
        settings.remove("apiInfo");
        settings.remove("authHash");
    }
    // remove from first version too
    {
        QSettings settings1("Windscribe", "Windscribe");
        settings1.remove("apiInfo");
        settings1.remove("authHash");
    }
}

bool ApiInfo::loadFromSettings()
{
    /*Q_ASSERT(threadId_ == QThread::currentThreadId());
    QSettings settings;
    QString s = settings.value("apiInfo", "").toString();
    if (!s.isEmpty())
    {
        QByteArray arr = simpleCrypt_.decryptToByteArray(s);
        ProtoApiInfo::ApiInfo protoApiInfo;
        if (!protoApiInfo.ParseFromArray(arr.data(), arr.size()))
        {
            return false;
        }

        sessionStatus_.initFromProtoBuf(protoApiInfo.session_status());

        locations_.clear();
        for (int i = 0; i < protoApiInfo.locations_size(); ++i)
        {
            Location location;
            location.initFromProtoBuf(protoApiInfo.locations(i));
            locations_ << location;
        }

        forceDisconnectNodes_.clear();
        serverCredentials_ = ServerCredentials(protoApiInfo.server_credentials());
        ovpnConfig_ = QString::fromStdString(protoApiInfo.ovpn_config());
        portMap_.initFromProtoBuf(protoApiInfo.port_map());
        staticIps_.initFromProtoBuf(protoApiInfo.static_ips());

        sessionStatus_.setRevisionHash(settings.value("revisionHash", "").toString());
        return true;
    }
    else*/
    {
        return false;
    }
}

void ApiInfo::mergeWindflixLocations()
{
    // Build a new list of server locations to merge, removing them from the old list.
    // Currently we merge all WindFlix locations into the corresponding global locations.
    QVector<Location> locationsToMerge;
    QMutableVectorIterator<Location> it(locations_);
    while (it.hasNext()) {
        Location &location = it.next();
        if (location.getName().startsWith("WINDFLIX")) {
            locationsToMerge.append(location);
            it.remove();
        }
    }
    if (!locationsToMerge.size())
        return;

    // Map city names to locations for faster lookups.
    QHash<QString, Location *> location_hash;
    for (auto &location: locations_) {
        for (int i = 0; i < location.groupsCount(); ++i)
        {
            const Group group = location.getGroup(i);
            location_hash.insert(location.getCountryCode() + group.getCity(), &location);
        }
    }

    // Merge the locations.
    QMutableVectorIterator<Location> itm(locationsToMerge);
    while (itm.hasNext()) {
        Location &location = itm.next();
        const auto country_code = location.getCountryCode();

        for (int i = 0; i < location.groupsCount(); ++i)
        {
            Group group = location.getGroup(i);
            group.setOverrideDnsHostName(location.getDnsHostName());

            auto target = location_hash.find(country_code + group.getCity());
            Q_ASSERT(target != location_hash.end());
            if (target != location_hash.end())
            {
                target.value()->addGroup(group);
            }
        }
        itm.remove();
    }
}

bool ApiInfo::ovpnConfigRefetchRequired() const
{
    return ovpnConfigSetTimestamp_.isValid() && (ovpnConfigSetTimestamp_.secsTo(QDateTime::currentDateTimeUtc()) >= 60*60*24);
}

} //namespace apiinfo
