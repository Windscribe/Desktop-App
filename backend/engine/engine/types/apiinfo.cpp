#include "apiinfo.h"
#include <QThread>
#include <QSettings>
#include <QDataStream>
#include <QDebug>
#include <QElapsedTimer>
#include "utils/logger.h"
#include "utils/utils.h"

const int typeIdApiNotifications = qRegisterMetaType<QSharedPointer<ApiNotifications> >("QSharedPointer<ApiNotifications>");

ApiInfo::ApiInfo() : simpleCrypt_(0x4572A4ACF31A31BA)
{

}

ApiInfo::~ApiInfo()
{
}

QSharedPointer<SessionStatus> ApiInfo::getSessionStatus() const
{
    return sessionStatus_;
}

void ApiInfo::setSessionStatus(const QSharedPointer<SessionStatus> &value)
{
    sessionStatus_ = value;
}

void ApiInfo::setServerLocations(const QVector<QSharedPointer<ServerLocation> > &value)
{
    serverLocations_ = value;
    processServerLocations();
}

QVector<QSharedPointer<ServerLocation> > ApiInfo::getServerLocations()
{
    return serverLocations_;
}

QStringList ApiInfo::getForceDisconnectNodes() const
{
    return forceDisconnectNodes_;
}

void ApiInfo::setForceDisconnectNodes(const QStringList &value)
{
    forceDisconnectNodes_ = value;
}

void ApiInfo::setServerCredentials(const ServerCredentials &serverCredentials)
{
    serverCredentials_ = serverCredentials;
}

const ServerCredentials &ApiInfo::getServerCredentials() const
{
    return serverCredentials_;
}

QByteArray ApiInfo::getOvpnConfig() const
{
    return ovpnConfig_;
}

void ApiInfo::setOvpnConfig(const QByteArray &value)
{
    ovpnConfig_ = value;
}

QString ApiInfo::getAuthHash() const
{
    return authHash_;
}

void ApiInfo::setAuthHash(const QString &authHash)
{
    authHash_ = authHash;
}

QSharedPointer<PortMap> ApiInfo::getPortMap() const
{
    return portMap_;
}

void ApiInfo::setPortMap(const QSharedPointer<PortMap> &portMap)
{
    portMap_ = portMap;
}

void ApiInfo::setStaticIpsLocation(QSharedPointer<StaticIpsLocation> &value)
{
    staticIps_ = value;
}

QSharedPointer<StaticIpsLocation> ApiInfo::getStaticIpsLocation() const
{
    return staticIps_;
}

void ApiInfo::saveToSettings()
{
    /*QSettings settings;

    QByteArray arr;
    {
        QDataStream stream(&arr, QIODevice::WriteOnly);

        stream << REVISION_VERSION;
        stream << ovpnConfig_;

        serverCredentials_.writeToStream(stream);

        int serverLocationsCount = serverLocations_.count();
        stream << serverLocationsCount;
        for (auto it = serverLocations_.begin(); it != serverLocations_.end(); ++it)
        {
            (*it)->writeToStream(stream);
        }

        // write 5 empty QString (old best location data, for compatiblity with old versions, not used now)
        QString emptyStr;
        for (int i = 0; i < 5; i++)
        {
            stream << emptyStr;
        }

        Q_ASSERT(!portMap_.isNull());
        portMap_->writeToStream(stream);

        Q_ASSERT(!sessionStatus_.isNull());
        sessionStatus_->writeToStream(stream);

        if (sessionStatus_->getStaticIpsCount() > 0 && !staticIps_.isNull())
        {
            staticIps_->writeToStream(stream);
        }
    }
    settings.setValue("apiInfo", simpleCrypt_.encryptToString(arr));
    if(sessionStatus_->isRevisionHashInitialized())
    {
        settings.setValue("revisionHash", sessionStatus_->getRevisionHash());
    }
    else
    {
        settings.remove("revisionHash");
    }

    settings.setValue("userId", sessionStatus_->userId);    // need for uninstaller program for open post uninstall webpage*/
}

void ApiInfo::removeFromSettings()
{
    QSettings settings;
    settings.remove("apiInfo");
}

bool ApiInfo::loadFromSettings()
{
    /*QSettings settings;
    QString s = settings.value("apiInfo", "").toString();
    if (!s.isEmpty())
    {
        QByteArray arr = simpleCrypt_.decryptToByteArray(s);
        QDataStream stream(&arr, QIODevice::ReadOnly);

        int revisionVersion;
        stream >> revisionVersion;

        if (revisionVersion != REVISION_VERSION)
        {
            return false;
        }

        stream >> ovpnConfig_;

        serverCredentials_.readFromStream(stream, revisionVersion);

        serverLocations_.clear();
        int serverLocationsCount;
        stream >> serverLocationsCount;
        for (int i = 0; i < serverLocationsCount; ++i)
        {
            QSharedPointer<ServerLocation> location(new ServerLocation());
            location->readFromStream(stream, revisionVersion);
            serverLocations_ << location;
        }
        processServerLocations();
        if (serverLocationsCount == 0)
        {
            return false;
        }

        // read 5 QString (old best location data, for compatiblity with old versions, not used now)
        QString emptyStr;
        for (int i = 0; i < 5; i++)
        {
            stream >> emptyStr;
        }

        QSharedPointer<PortMap> portMap(new PortMap());
        portMap->readFromStream(stream);
        portMap_ = portMap;

        QSharedPointer<SessionStatus> sessionStatus(new SessionStatus());
        sessionStatus->readFromStream(stream, revisionVersion);
        sessionStatus_ = sessionStatus;

        if (settings.contains("revisionHash"))
        {
            sessionStatus_->setRevisionHash(settings.value("revisionHash").toString());
        }
        else
        {
            return false;
        }

        if (sessionStatus_->staticIps > 0)
        {
            staticIps_ = QSharedPointer<StaticIpsLocation>(new StaticIpsLocation());
            staticIps_->readFromStream(stream, revisionVersion);
        }

        return true;
    }
    else
    {
        return false;
    }*/
    return false;
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
    // Build a new list of server locations to merge, removing them from the old list.
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
    }
}

/*void SessionStatus::writeToStream(QDataStream &stream)
{
    stream << isPremium;
    stream << status;
    stream << rebill;
    stream << billingPlanId;
    //stream << premiumExpireDate;
    stream << trafficUsed;
    stream << trafficMax;
    stream << userId;
    stream << username;
    stream << staticIps;
    stream << email;
    stream << emailStatus;
    stream << premiumExpireDateStr;
}

void SessionStatus::readFromStream(QDataStream &stream, int revision)
{
    stream >> isPremium;
    stream >> status;
    stream >> rebill;
    if (revision >= 7)
    {
        stream >> billingPlanId;
    }
    else
    {
        billingPlanId = INT_MIN;
    }
    //stream >> premiumExpireDate;
    stream >> trafficUsed;
    stream >> trafficMax;
    stream >> userId;
    stream >> username;

    if (revision >= 8)
    {
        stream >> staticIps;
    }

    if (revision >= 10)
    {
        stream >> email;
        stream >> emailStatus;
    }
    if (revision >= 11)
    {
        stream >> premiumExpireDateStr;
    }
}*/

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
