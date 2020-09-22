#include "locationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>
#include "utils/logger.h"

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager) : QObject(parent),
    pingIpsController_(this, stateController, networkStateManager)
{
    connect(&pingIpsController_, SIGNAL(pingInfoChanged(QString,int, bool)), SLOT(onPingInfoChanged(QString,int, bool)));
    connect(&pingIpsController_, SIGNAL(needIncrementPingIteration()), SLOT(onNeedIncrementPingIteration()));
    pingStorage_.incIteration();

    if (bestLocation_.isValid())
    {
        qCDebug(LOG_BEST_LOCATION) << "Best location loaded from settings: " << bestLocation_.getId();
    }
    else
    {
        qCDebug(LOG_BEST_LOCATION) << "No saved best location in settings";
    }
}

void LocationsModel::setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps)
{
    locations_ = locations;
    staticIps_ = staticIps;

    // ping stuff
    QVector<PingIpInfo> ips;
    QStringList stringListIps;
    for (const apiinfo::Location &l : locations)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            PingIpInfo pii;
            pii.ip = l.getGroup(i).getPingIp();
            pii.pingType = PingHost::PING_TCP;
            ips << pii;
            stringListIps << pii.ip;
        }
    }

    // handle static ips location
    if (staticIps_.getIpsCount() > 0)
    {
        for (int i = 0; i < staticIps_.getIpsCount(); ++i)
        {
            const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
            PingIpInfo pii;
            pii.ip = sid.getPingIp();
            pii.pingType = PingHost::PING_TCP;
            ips << pii;
            stringListIps << pii.ip;
        }
    }

    pingStorage_.updateNodes(stringListIps);
    pingIpsController_.updateIps(ips);

    generateLocationsUpdated();
}

void LocationsModel::clear()
{
    locations_.clear();
    staticIps_ = apiinfo::StaticIps();
    pingIpsController_.updateIps(QVector<PingIpInfo>());
    QSharedPointer<QVector<locationsmodel::LocationItem> > empty(new QVector<locationsmodel::LocationItem>());
    emit locationsUpdated(empty);
}

void LocationsModel::setProxySettings(const ProxySettings &proxySettings)
{
    pingIpsController_.setProxySettings(proxySettings);
}

void LocationsModel::disableProxy()
{
    pingIpsController_.disableProxy();
}

void LocationsModel::enableProxy()
{
    pingIpsController_.enableProxy();
}

QSharedPointer<MutableLocationInfo> LocationsModel::getMutableLocationInfoById(const LocationID &locationId)
{
    if (locationId.getId() == LocationID::STATIC_IPS_LOCATION_ID)
    {
        if (staticIps_.getIpsCount() > 0)
        {
            for (int i = 0; i < staticIps_.getIpsCount(); ++i)
            {
                const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
                LocationID staticIpLocationId = LocationID::createFromStaticIpsLocation(sid.cityName, sid.staticIp);

                if (staticIpLocationId == locationId)
                {
                    QVector< QSharedPointer<BaseNode> > nodes;

                    QStringList ips;
                    ips << sid.nodeIP1 << sid.nodeIP2 << sid.nodeIP3;
                    nodes << QSharedPointer<BaseNode>(new StaticLocationNode(ips, sid.hostname, sid.dnsHostname, sid.username, sid.password, sid.getAllStaticIpIntPorts()));

                    QSharedPointer<MutableLocationInfo> mli(new MutableLocationInfo(locationId, sid.cityName + " - " + sid.staticIp, nodes, -1, ""));
                    return mli;
                }
            }
        }
    }
    else
    {
        for (const apiinfo::Location &l : locations_)
        {
            if (l.getId() == locationId.getId())
            {
                for (int i = 0; i < l.groupsCount(); ++i)
                {
                    const apiinfo::Group group = l.getGroup(i);
                    if (LocationID::createFromApiLocation(l.getId(), group.getId()).getCity() == locationId.getCity())
                    {
                        QVector< QSharedPointer<BaseNode> > nodes;
                        for (int n = 0; n < group.getNodesCount(); ++n)
                        {
                            const apiinfo::Node &apiInfoNode = group.getNode(n);
                            QStringList ips;
                            ips << apiInfoNode.getIp(0) << apiInfoNode.getIp(1) << apiInfoNode.getIp(2);
                            nodes << QSharedPointer<BaseNode>(new ApiLocationNode(ips, apiInfoNode.getHostname(), apiInfoNode.getWeight()));
                        }


                        QSharedPointer<MutableLocationInfo> mli(new MutableLocationInfo(locationId, group.getCity() + " - " + group.getNick(), nodes, -1, l.getDnsHostName()));
                        return mli;
                    }
                }
            }
        }
    }
    return NULL;
}

void LocationsModel::onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState)
{
    pingStorage_.setNodePing(ip, timems, isFromDisconnectedState);

    bool isAllNodesHaveCurIteration;
    bool isAllNodesInDisconnectedState;
    pingStorage_.getState(isAllNodesHaveCurIteration, isAllNodesInDisconnectedState);

    if (isAllNodesHaveCurIteration)
    {
        detectBestLocation(isAllNodesInDisconnectedState);
    }

    for (const apiinfo::Location &l : locations_)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            const apiinfo::Group group = l.getGroup(i);
            if (group.getPingIp() == ip)
            {
                emit locationPingTimeChanged(LocationID::createFromApiLocation(l.getId(), group.getId()), timems);
            }
        }
    }

    // handle static ips location
    if (staticIps_.getIpsCount() > 0)
    {
        for (int i = 0; i < staticIps_.getIpsCount(); ++i)
        {
            const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
            if (sid.getPingIp() == ip)
            {
                emit locationPingTimeChanged(LocationID::createFromStaticIpsLocation(sid.cityName, sid.staticIp), timems);
            }
        }
    }
}

void LocationsModel::onNeedIncrementPingIteration()
{
    pingStorage_.incIteration();
}

void LocationsModel::detectBestLocation(bool isAllNodesInDisconnectedState)
{
    int minLatency = INT_MAX;
    int min_ind = -1;

    qCDebug(LOG_BEST_LOCATION) << "LocationsModel::detectBestLocation, isAllNodesInDisconnectedState=" << isAllNodesInDisconnectedState;

    int prevBestLocationLatency = 0;

    int ind = 0;
    for (const apiinfo::Location &l : locations_)
    {
        int latency = calcLatency(l);

        if (bestLocation_.isValid() && l.getId() == bestLocation_.getId())
        {
            prevBestLocationLatency = latency;
        }

        if (latency != -1 && latency < minLatency)
        {
            minLatency = latency;
            min_ind = ind;
        }
        ind++;
    }

    int prevBestLocationId = -100; // LocationID::EMPTY_LOCATION;
    if (bestLocation_.isValid())
    {
        prevBestLocationId = bestLocation_.getId();
    }

     qCDebug(LOG_BEST_LOCATION) << "prevBestLocationId=" << prevBestLocationId << "; prevBestLocationLatency=" << prevBestLocationLatency;

    if (min_ind != -1)      // new best location found
    {
        qCDebug(LOG_BEST_LOCATION) << "Detected min latency=" << minLatency << "; id=" << locations_[min_ind].getId();

        // check whether best location needs to be changed
        if (!bestLocation_.isValid())
        {
            bestLocation_.set(locations_[min_ind].getId(), true, isAllNodesInDisconnectedState);
        }
        else // if best location is valid
        {
            if (!bestLocation_.isDetectedFromThisAppStart())
            {
                bestLocation_.set(locations_[min_ind].getId(), true, isAllNodesInDisconnectedState);
            }
            else if (bestLocation_.isDetectedWithDisconnectedIps())
            {
                if (isAllNodesInDisconnectedState)
                {
                    // check ping time changed more than 10% compared to prev best location
                    if ((double)minLatency < ((double)prevBestLocationLatency * 0.9))
                    {
                        bestLocation_.set(locations_[min_ind].getId(), true, isAllNodesInDisconnectedState);
                    }
                }
            }
            else if (!bestLocation_.isDetectedWithDisconnectedIps())
            {
                if (isAllNodesInDisconnectedState)
                {
                    bestLocation_.set(locations_[min_ind].getId(), true, isAllNodesInDisconnectedState);
                }
            }
        }
    }

    // send the signal to the GUI only if the location has actually changed
    if (bestLocation_.isValid() && prevBestLocationId != bestLocation_.getId())
    {
        qCDebug(LOG_BEST_LOCATION) << "Best location changed to " << bestLocation_.getId();
        generateLocationsUpdated();
    }
}

void LocationsModel::generateLocationsUpdated()
{
    QSharedPointer <QVector<LocationItem> > items(new QVector<LocationItem>());

    bool isBestLocationInserted = false;
    for (const apiinfo::Location &l : locations_)
    {
        LocationItem item;
        item.id = l.getId();
        item.name = l.getName();
        item.countryCode = l.getCountryCode();
        item.isPremiumOnly = l.isPremiumOnly();
        item.p2p = l.getP2P();

        for (int i = 0; i < l.groupsCount(); ++i)
        {
            const apiinfo::Group group = l.getGroup(i);
            CityItem city;
            city.cityId = LocationID::createFromApiLocation(l.getId(), group.getId()).getCity();
            city.city = group.getCity();
            city.nick = group.getNick();
            city.isPro = group.isPro();
            city.pingTimeMs = pingStorage_.getNodeSpeed(group.getPingIp());
            city.isDisabled = group.isDisabled();
            item.cities << city;
        }

        *items << item;

        if (!isBestLocationInserted && bestLocation_.isValid() && bestLocation_.getId() == item.id)
        {
            item.id = LocationID::BEST_LOCATION_ID;
            item.name = "Best Location";
            items->insert(0,item);
            isBestLocationInserted = true;
        }
    }

    // use first location as best location
    if (!isBestLocationInserted)
    {
        if (items->count() > 0)
        {
            LocationItem firstItem = items->at(0);
            bestLocation_.set(firstItem.id, false, false);
            firstItem.id = LocationID::BEST_LOCATION_ID;
            firstItem.name = "Best Location";
            items->insert(0,firstItem);

            qCDebug(LOG_BEST_LOCATION) << "Best location chosen as the first location:" << bestLocation_.getId();
        }
    }

    // add static ips location
    if (staticIps_.getIpsCount() > 0)
    {
        LocationItem item;

        item.id = LocationID::STATIC_IPS_LOCATION_ID;
        item.name = QObject::tr("Static IPs");
        item.countryCode = "STATIC_IPS";
        item.isPremiumOnly = false;
        item.p2p = 1;
        item.staticIpsDeviceName = staticIps_.getDeviceName();

        for (int i = 0; i < staticIps_.getIpsCount(); ++i)
        {
            const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
            CityItem city;
            city.cityId = LocationID::createFromStaticIpsLocation(sid.cityName, sid.staticIp).getCity();
            city.city = sid.cityName;
            city.pingTimeMs = pingStorage_.getNodeSpeed(sid.getPingIp());
            city.isPro = true;
            city.isDisabled = false;
            city.staticIp = sid.staticIp;
            city.staticIpType = sid.type;

            item.cities << city;
        }

        *items << item;
    }

    emit locationsUpdated(items);
}

int LocationsModel::calcLatency(const apiinfo::Location &l)
{
    double sumLatency = 0;
    int cnt = 0;

    for (int i = 0; i < l.groupsCount(); ++i)
    {
        const apiinfo::Group group = l.getGroup(i);
        if (!group.isDisabled())
        {
            PingTime pingTime = pingStorage_.getNodeSpeed(group.getPingIp());
            if (pingTime == PingTime::NO_PING_INFO)
            {
                sumLatency += 200;      // we assume a maximum ping time for three bars
            }
            else if (pingTime == PingTime::PING_FAILED)
            {
                sumLatency += 2000;    // 2000 - max ping interval
            }
            else
            {
                sumLatency += pingTime.toInt();
            }
            cnt++;
        }
    }
    if (cnt > 0)
    {
        return sumLatency / (double)cnt;
    }
    else
    {
        return PingTime::PING_FAILED;
    }
}

} //namespace locationsmodel
