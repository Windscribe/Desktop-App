#include "apiinfo.h"
#include <QThread>
#include <QSettings>
#include "utils/logger.h"
#include "utils/utils.h"
#include "ipc/generated_proto/apiinfo.pb.h"

namespace apiinfo {

ApiInfo::ApiInfo() : simpleCrypt_(0x4572A4ACF31A31BA)
{
    threadId_ = QThread::currentThreadId();
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
}

void ApiInfo::setLocations(const QVector<Location> &value)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    locations_ = value;
    processServerLocations();
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
}

QString ApiInfo::getAuthHash() const
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    return authHash_;
}

void ApiInfo::setAuthHash(const QString &authHash)
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
    authHash_ = authHash;
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
    Q_ASSERT(threadId_ == QThread::currentThreadId());

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
    }

    settings.setValue("userId", sessionStatus_.getUserId());    // need for uninstaller program for open post uninstall webpage*/
}

void ApiInfo::removeFromSettings()
{
    QSettings settings;
    settings.remove("apiInfo");
}

bool ApiInfo::loadFromSettings()
{
    Q_ASSERT(threadId_ == QThread::currentThreadId());
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
        processServerLocations();

        sessionStatus_.setRevisionHash(settings.value("revisionHash", "").toString());
        return true;
    }
    else
    {
        return false;
    }
}

/*void ApiInfo::debugSaveToFile(const QString &filename)
{
    QByteArray arr;
    {
        QDataStream stream(&arr, QIODevice::WriteOnly);

        stream << REVISION_VERSION;
        stream << ovpnConfig_;
        stream << radiusUsername_;
        stream << radiusPassword_;

        int serverLocationsCount = serverLocations_.count();
        stream << serverLocationsCount;
        for (auto it = serverLocations_.begin(); it != serverLocations_.end(); ++it)
        {
            (*it)->writeToStream(stream);
        }

        Q_ASSERT(!bestLocation_.isNull());
        bestLocation_->writeToStream(stream);

        Q_ASSERT(!portMap_.isNull());
        portMap_->writeToStream(stream);

        Q_ASSERT(!sessionStatus_.isNull());
        sessionStatus_->writeToStream(stream);
    }
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(arr);
        file.close();
    }
}

void ApiInfo::debugLoadFromFile(const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray arr = file.readAll();

        QDataStream stream(&arr, QIODevice::ReadOnly);

        int revisionVersion;
        stream >> revisionVersion;
        stream >> ovpnConfig_;
        stream >> radiusUsername_;
        stream >> radiusPassword_;

        serverLocations_.clear();
        int serverLocationsCount;
        stream >> serverLocationsCount;
        for (int i = 0; i < serverLocationsCount; ++i)
        {
            QSharedPointer<ServerLocation> location(new ServerLocation());
            location->readFromStream(stream, revisionVersion);
            serverLocations_ << location;
        }
        if (serverLocationsCount == 0)
        {
            return;
        }

        QSharedPointer<BestLocation> best(new BestLocation());
        best->readFromStream(stream);
        bestLocation_ = best;

        QSharedPointer<PortMap> portMap(new PortMap());
        portMap->readFromStream(stream);
        portMap_ = portMap;

        QSharedPointer<SessionStatus> sessionStatus(new SessionStatus());
        sessionStatus->readFromStream(stream);
        sessionStatus_ = sessionStatus;

        mergeServerLocationsWithBestLocation();

        file.close();
    }
}*/

void ApiInfo::processServerLocations()
{
    /*// Build a new list of server locations to merge, removing them from the old list.
    // Currently we merge all WindFlix locations into the corresponding global locations.
    using LocationPtr = QSharedPointer<ServerLocation>;
    using LocationsType = QVector<LocationPtr>;
    LocationsType locationsToMerge;
    QMutableVectorIterator<LocationPtr> it(serverLocations_);
    while (it.hasNext()) {
        LocationPtr &location = it.next();
        if (location->getName().startsWith("WINDFLIX")) {
            locationsToMerge.append(location);
            it.remove();
        }
    }
    if (!locationsToMerge.size())
        return;

    // Utility function to extract city name part for location matching.
    auto getRealCityName = [](const QString &str) {
        auto parts = str.split(" - ");
        return parts.size() > 0 ? parts[0].trimmed() : str.trimmed();
    };

    // Map city names to locations for faster lookups.
    QHash<QString, LocationPtr> location_hash;
    for (auto &location: serverLocations_) {
        const auto title = location->getName();
        if (title == QT_TR_NOOP("Best Location"))
            continue;
        const auto cities = location->getCities();
        for (const auto &city : cities)
            location_hash.insert(location->getCountryCode() + getRealCityName(city), location);
        const auto pro_datacentres = location->getProDataCenters();
        for (const auto &pro_datacentre : pro_datacentres)
            location_hash.insert(location->getCountryCode() + getRealCityName(pro_datacentre),
                                 location);
    }

    // Merge the locations.
    QMutableVectorIterator<LocationPtr> itm(locationsToMerge);
    while (itm.hasNext()) {
        LocationPtr &location = itm.next();
        const auto country_code = location->getCountryCode();
        const auto nodes = location->getNodes();
        for (const auto &n: nodes) {
            auto target = location_hash.value(country_code + getRealCityName(n.getCityName()));
            Q_ASSERT(!target.isNull());
            if (!target.isNull())
                target->appendNode(n);
        }
        const auto pro_datacentres = location->getProDataCenters();
        for (const auto &pro_datacentre : pro_datacentres) {
            auto target = location_hash.value(country_code + getRealCityName(pro_datacentre));
            Q_ASSERT(!target.isNull());
            if (!target.isNull())
                target->appendProDataCentre(pro_datacentre);
        }
        itm.remove();
    }*/
}

/*bool ServerNode::isEqualIpsServerNodes(const QVector<ServerNode> &n1, const QVector<ServerNode> &n2)
{
    if (n1.size() != n2.size())
    {
        return false;
    }
    else
    {
        for (int i = 0; i < n1.size(); ++i)
        {
            if (n1[i].ip[0] != n2[i].ip[0] || n1[i].ip[1] != n2[i].ip[1] || n1[i].ip[2] != n2[i].ip[2])
            {
                return false;
            }
        }
        return true;
    }
}*/

} //namespace apiinfo
