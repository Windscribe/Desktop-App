#include "location.h"

#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include "../types/locationid.h"

const int typeIdLocation = qRegisterMetaType<ApiInfo::Location>("ApiInfo::Location");
const int typeIdLocations = qRegisterMetaType<QVector<ApiInfo::Location>>("QVector<ApiInfo::Location>");

namespace ApiInfo {


bool Location::initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes)
{
    if (!obj.contains("id") || !obj.contains("name") || !obj.contains("country_code") ||
            !obj.contains("premium_only") || !obj.contains("p2p") || !obj.contains("groups"))
    {
        d->isValid_ = false;
        return false;
    }

    d->id_ = obj["id"].toInt();
    d->name_ = obj["name"].toString();
    d->countryCode_ = obj["country_code"].toString();
    d->premiumOnly_ = obj["premium_only"].toInt();
    d->p2p_ = obj["p2p"].toInt();
    if (obj.contains("dns_hostname"))
    {
        d->dnsHostName_ = obj["dns_hostname"].toString();
    }

    const auto groupsArray = obj["groups"].toArray();
    for (const QJsonValue &serverGroupValue : groupsArray)
    {
        QJsonObject objServerGroup = serverGroupValue.toObject();
        Group group;
        if (!group.initFromJson(objServerGroup, forceDisconnectNodes))
        {
            d->isValid_ = false;
            return false;
        }
        d->groups_ << group;
    }

    d->isValid_ = true;
    d->type_ = SERVER_LOCATION_DEFAULT;
    return true;
}

void Location::initFromProtoBuf(const ProtoApiInfo::Location &l)
{

}

QStringList Location::getAllIps() const
{
    Q_ASSERT(d->isValid_);
    QStringList ips;
    for (const Group &g : d->groups_)
    {
        ips << g.getAllIps();
    }
    return ips;
}


/*
void ServerLocation::transformToBestLocation(int selectedNodeIndForBestLocation, int bestLocationPingTimeMs, int bestLocationId)
{
    id_ = LocationID::BEST_LOCATION_ID;
    bestLocationId_ = bestLocationId;
    name_ = QObject::tr("Best Location");
    type_ = SERVER_LOCATION_BEST;
    selectedNodeIndForBestLocation_ = selectedNodeIndForBestLocation;
    bestLocationPingTimeMs_ = bestLocationPingTimeMs;
}

void ServerLocation::transformToCustomOvpnLocation(QVector<ServerNode> &nodes)
{
    isValid_ = true;
    type_ = SERVER_LOCATION_CUSTOM_CONFIG;

    countryCode_ = "Custom_Configs";
    nodes_ = nodes;
    p2p_ = 1;
    premiumOnly_ = false;
    id_ = LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID;
    name_ = QObject::tr("Custom Configs");
    makeInternalStates();
}

/*void ServerLocation::writeToStream(QDataStream &stream)
{
    Q_ASSERT(isValid_);
    stream << id_;
    stream << name_;
    stream << countryCode_;
    // should be removed in the future
    int notUsedStatus = 1;
    stream << notUsedStatus;
    stream << premiumOnly_;
    stream << p2p_;
    stream << dnsHostName_;

    stream << nodes_.count();
    Q_FOREACH(const ServerNode &node, nodes_)
    {
        node.writeToStream(stream);
    }

    stream << proDataCenters_;
    bool notUsedForceExpand = true;
    stream << notUsedForceExpand;

    bool isBestLocation = (type_ == SERVER_LOCATION_BEST);
    stream << isBestLocation;

    stream << bestLocationId_;
    stream << selectedNodeIndForBestLocation_;
    stream << bestLocationPingTimeMs_;
}

void ServerLocation::readFromStream(QDataStream &stream, int revision)
{
    stream >> id_;
    stream >> name_;
    stream >> countryCode_;
    int notUsedStatus;
    stream >> notUsedStatus;
    stream >> premiumOnly_;
    stream >> p2p_;
    stream >> dnsHostName_;

    nodes_.clear();
    int nodesCount;
    stream >> nodesCount;
    for (int i = 0; i < nodesCount; ++i)
    {
        ServerNode node;
        node.readFromStream(stream);
        nodes_ << node;
    }

    proDataCenters_.clear();
    if (revision >= 2)
    {
        stream >> proDataCenters_;
    }
    if (revision >= 4)
    {
        bool notUsedForceExpand;
        stream >> notUsedForceExpand;
    }

    bool isBestLocation = false;

    if (revision >= 5)
    {
        stream >> isBestLocation;
        stream >> bestLocationId_;
        stream >> selectedNodeIndForBestLocation_;
        stream >> bestLocationPingTimeMs_;
    }

    isValid_ = true;

    if (isBestLocation)
    {
        type_ = SERVER_LOCATION_BEST;
    }
    else
    {
        type_ = SERVER_LOCATION_DEFAULT;
    }

    makeInternalStates();
}

int ServerLocation::getId() const
{
    Q_ASSERT(isValid_);
    return id_;
}

int ServerLocation::nodesCount() const
{
    Q_ASSERT(isValid_);
    return nodes_.count();
}
*/

/*QString ServerLocation::getCountryCode() const
{
    Q_ASSERT(isValid_);
    return countryCode_;
}

QString ServerLocation::getName() const
{
    Q_ASSERT(isValid_);
    return name_;
}

int ServerLocation::getP2P() const
{
    Q_ASSERT(isValid_);
    return p2p_;
}

QString ServerLocation::getDnsHostname() const
{
    Q_ASSERT(isValid_);
    return dnsHostName_;
}

bool ServerLocation::isPremiumOnly() const
{
    Q_ASSERT(isValid_);
    return premiumOnly_ == 1;
}

bool ServerLocation::isExistsHostname(const QString &hostname) const
{
    Q_ASSERT(isValid_);
    for (int i = 0; i < nodes_.count(); ++i)
    {
        if (nodes_[i].getHostname() == hostname)
        {
            return true;
        }
    }
    return false;
}

int ServerLocation::nodeIndByHostname(const QString &hostname) const
{
    Q_ASSERT(isValid_);
    for (int i = 0; i < nodes_.count(); ++i)
    {
        if (nodes_[i].getHostname() == hostname)
        {
            return i;
        }
    }
    return -1;
}

QStringList ServerLocation::getCities()
{
    Q_ASSERT(isValid_);
    QStringList strs;
    for (auto it = citiesMap_.begin(); it != citiesMap_.end(); ++it)
    {
        strs << it.key();
    }
    return strs;
}

const QStringList &ServerLocation::getProDataCenters() const
{
    Q_ASSERT(isValid_);
    return proDataCenters_;
}

void ServerLocation::appendProDataCentre(const QString &name)
{
    Q_ASSERT(isValid_);
    proDataCenters_.append(name);
}

QVector<ServerNode> &ServerLocation::getNodes()
{
    Q_ASSERT(isValid_);
    return nodes_;
}

/*QVector<ServerNode> ServerLocation::getCityNodes(const QString &cityname)
{
    QVector<ServerNode> cityNodes;

    auto it = citiesMap_.find(cityname);
    if (it != citiesMap_.end())
    {
        Q_FOREACH(int ind, it.value()->nodesInds)
        {
            cityNodes << nodes_[ind];
        }
    }

    return cityNodes;
}

void ServerLocation::appendNode(const ServerNode &node)
{
    nodes_ << node;
    makeInternalStates();
}

QString ServerLocation::getStaticIpsDeviceName() const
{
    return staticIpsDeviceName_;
}

int ServerLocation::getBestLocationId() const
{
    return bestLocationId_;
}

int ServerLocation::getBestLocationSelectedNodeInd() const
{
    Q_ASSERT(type_ == SERVER_LOCATION_BEST);
    return selectedNodeIndForBestLocation_;
}

int ServerLocation::getBestLocationPingTimeMs() const
{
    Q_ASSERT(type_ == SERVER_LOCATION_BEST);
    return bestLocationPingTimeMs_;
}*/

/*bool ServerLocation::isEqual(ServerLocation *other)
{
    Q_ASSERT(other != NULL);
    if (id_ != other->id_ || name_ != other->name_ || countryCode_ != other->countryCode_ ||
        premiumOnly_ != other->premiumOnly_ || p2p_ != other->p2p_ ||
        dnsHostName_ != other->dnsHostName_ || proDataCenters_ != other->proDataCenters_)
    {
        return false;
    }

    if (isValid_ != other->isValid_ || type_ != other->type_ ||
        selectedNodeIndForBestLocation_ != other->selectedNodeIndForBestLocation_ ||
        bestLocationPingTimeMs_ != other->bestLocationPingTimeMs_ || bestLocationId_ != other->bestLocationId_)
    {
        return false;
    }

    //compare nodes
    if (nodes_.count() != other->nodes_.count())
    {
        return false;
    }
    for (int i = 0; i < nodes_.count(); ++i)
    {
        if (!nodes_[i].isEqual(other->nodes_[i]))
        {
            return false;
        }
    }
    return true;
}*/

// grouping cities, making citiesMap_ data
/*void ServerLocation::makeInternalStates()
{
    int nodeInd = 0;
    Q_FOREACH(const ServerNode &node, nodes_)
    {
        if (!node.getCityName().isEmpty())
        {
            QMap<QString, QSharedPointer<CityNodes> >::iterator it = citiesMap_.find(node.getCityName());
            if (it != citiesMap_.end())
            {
                it.value()->nodesInds.push_back(nodeInd);
            }
            else
            {
                QSharedPointer<CityNodes> cityNodes(new CityNodes);
                cityNodes->nodesInds.push_back(nodeInd);

                citiesMap_[node.getCityName()] = cityNodes;
            }
        }
        else
        {
            Q_ASSERT(false);
        }
        nodeInd++;
    }
}*/

/*QVector<QSharedPointer<ServerLocation> > makeCopyOfServerLocationsVector(QVector<QSharedPointer<ServerLocation> > &serverLocations)
{
    QVector<QSharedPointer<ServerLocation> > copyServerlocations;
    Q_FOREACH(QSharedPointer<ServerLocation> s, serverLocations)
    {
        QSharedPointer<ServerLocation> copySl(new ServerLocation);
        *copySl = *s;
        copyServerlocations << copySl;
    }
    return copyServerlocations;
}*/

} // namespace ApiInfo
