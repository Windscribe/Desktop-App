#include "apilocationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>
#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "mutablelocationinfo.h"
#include "nodeselectionalgorithm.h"

namespace locationsmodel {

ApiLocationsModel::ApiLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager, PingHost *pingHost) : QObject(parent),
    pingStorage_("pingStorage"),
    pingIpsController_(this, stateController, networkDetectionManager, pingHost, "ping_log.txt")
{
    connect(&pingIpsController_, SIGNAL(pingInfoChanged(QString,int, bool)), SLOT(onPingInfoChanged(QString,int, bool)));
    connect(&pingIpsController_, SIGNAL(needIncrementPingIteration()), SLOT(onNeedIncrementPingIteration()));
    pingStorage_.incIteration();

    if (bestLocation_.isValid())
    {
        qCDebug(LOG_BEST_LOCATION) << "Best location loaded from settings: " << bestLocation_.getId().getHashString();
    }
    else
    {
        qCDebug(LOG_BEST_LOCATION) << "No saved best location in settings";
    }
}

void ApiLocationsModel::setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps)
{
    if (!isChanged(locations, staticIps))
    {
        return;
    }

    locations_ = locations;
    staticIps_ = staticIps;

    whitelistIps();

    // ping stuff
    QVector<PingIpInfo> ips;
    QStringList stringListIps;
    for (const apiinfo::Location &l : locations)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            apiinfo::Group group = l.getGroup(i);
            ips << PingIpInfo(group.getPingIp(), group.getCity(), group.getNick(), PingHost::PING_TCP);
            stringListIps << group.getPingIp();
        }
    }

    // handle static ips location
    for (int i = 0; i < staticIps_.getIpsCount(); ++i)
    {
        const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
        QString pingIp = sid.getPingIp();
        ips << PingIpInfo(pingIp, sid.name, "staticIP", PingHost::PING_TCP);
        stringListIps << pingIp;
    }

    pingStorage_.updateNodes(stringListIps);
    pingIpsController_.updateIps(ips);
    sendLocationsUpdated();
}

void ApiLocationsModel::clear()
{
    locations_.clear();
    staticIps_ = apiinfo::StaticIps();
    pingIpsController_.updateIps(QVector<PingIpInfo>());
    QSharedPointer<QVector<types::Location> > empty(new QVector<types::Location>());
    Q_EMIT locationsUpdated(LocationID(), QString(),  empty);
}

QSharedPointer<BaseLocationInfo> ApiLocationsModel::getMutableLocationInfoById(const LocationID &locationId)
{
    LocationID modifiedLocationId = locationId;

    if (locationId.isStaticIpsLocation())
    {
        if (staticIps_.getIpsCount() > 0)
        {
            for (int i = 0; i < staticIps_.getIpsCount(); ++i)
            {
                const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
                LocationID staticIpLocationId = LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp);

                if (staticIpLocationId == locationId)
                {
                    QVector< QSharedPointer<const BaseNode> > nodes;

                    QStringList ips;
                    for (auto it : sid.nodeIPs)
                    {
                        ips << it;
                    }
                    nodes << QSharedPointer<BaseNode>(new StaticLocationNode(ips, sid.hostname, sid.wgPubKey, sid.wgIp, sid.dnsHostname, sid.username, sid.password, sid.getAllStaticIpIntPorts()));

                    QSharedPointer<BaseLocationInfo> bli(new MutableLocationInfo(locationId, sid.cityName + " - " + sid.staticIp, nodes, 0, "", sid.ovpnX509));
                    return bli;
                }
            }
        }
    }
    else if (locationId.isBestLocation())
    {
        modifiedLocationId = locationId.bestLocationToApiLocation();
    }

    for (const apiinfo::Location &l : locations_)
    {
        if (LocationID::createTopApiLocationId(l.getId()) == modifiedLocationId.toTopLevelLocation())
        {
            for (int i = 0; i < l.groupsCount(); ++i)
            {
                const apiinfo::Group group = l.getGroup(i);
                if (LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()) == modifiedLocationId)
                {
                    QVector< QSharedPointer<const BaseNode> > nodes;
                    for (int n = 0; n < group.getNodesCount(); ++n)
                    {
                        const apiinfo::Node &apiInfoNode = group.getNode(n);
                        QStringList ips;
                        ips << apiInfoNode.getIp(0) << apiInfoNode.getIp(1) << apiInfoNode.getIp(2);
                        nodes << QSharedPointer<const ApiLocationNode>(new ApiLocationNode(ips, apiInfoNode.getHostname(), apiInfoNode.getWeight(), group.getWgPubKey()));
                    }

                    // once API server list is updated so that the old WINDFLIX locations' dns_hostname matches that of the containing region this code can be removed
                    QString dnsHostname;
                    if (!group.getDnsHostName().isEmpty())
                    {
                        dnsHostname = group.getDnsHostName();
                        qCDebug(LOG_BASIC) << "Overriding DNS hostname for old WINDFLIX location with: " << dnsHostname;
                    }
                    else
                    {
                        dnsHostname =  l.getDnsHostName();
                    }

                    int selectedNode = NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(nodes);
                    QSharedPointer<BaseLocationInfo> bli(new MutableLocationInfo(modifiedLocationId, group.getCity() + " - " + group.getNick(), nodes, selectedNode,dnsHostname, group.getOvpnX509()));
                    return bli;
                }
            }
        }
    }

    return NULL;
}

void ApiLocationsModel::onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState)
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
                Q_EMIT locationPingTimeChanged(LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()), timems);
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
                Q_EMIT locationPingTimeChanged(LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp), timems);
            }
        }
    }
}

void ApiLocationsModel::onNeedIncrementPingIteration()
{
    pingStorage_.incIteration();
}

void ApiLocationsModel::detectBestLocation(bool isAllNodesInDisconnectedState)
{
    int minLatency = INT_MAX;
    LocationID locationIdWithMinLatency;

    // Commented debug entry out as this method is potentially called every minute and we don't
    // need to flood the log with this info.
    // qCDebug(LOG_BEST_LOCATION) << "LocationsModel::detectBestLocation, isAllNodesInDisconnectedState=" << isAllNodesInDisconnectedState;

    int prevBestLocationLatency = INT_MAX;

    int ind = 0;
    for (const apiinfo::Location &l : locations_)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            const apiinfo::Group group = l.getGroup(i);

            if (group.isDisabled())
            {
                continue;
            }

            LocationID lid = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick());
            int latency = pingStorage_.getNodeSpeed(group.getPingIp()).toInt();

            // we assume a maximum ping time for three bars when no ping info
            if (latency == PingTime::NO_PING_INFO)
            {
                latency = PingTime::LATENCY_STEP1;
            }
            else if (latency == PingTime::PING_FAILED)
            {
                latency = PingTime::MAX_LATENCY_FOR_PING_FAILED;
            }

            if (bestLocation_.isValid() && lid == bestLocation_.getId())
            {
                prevBestLocationLatency = latency;
            }
            if (latency != PingTime::PING_FAILED && latency < minLatency)
            {
                minLatency = latency;
                locationIdWithMinLatency = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()) ;
            }
        }

        ind++;
    }

    LocationID prevBestLocationId;
    if (bestLocation_.isValid())
    {
        prevBestLocationId = bestLocation_.getId();
        // Commented debug entry out as this method is potentially called every minute and we don't
        // need to flood the log with this info.  We will log it if the location actually changes.
        //qCDebug(LOG_BEST_LOCATION) << "prevBestLocationId=" << prevBestLocationId.getHashString() << "; prevBestLocationLatency=" << prevBestLocationLatency;
    }


    if (locationIdWithMinLatency.isValid())      // new best location found
    {
        // Commented debug entry out as this method is potentially called every minute and we don't
        // need to flood the log with this info.  We will log it if the location actually changes.
        //qCDebug(LOG_BEST_LOCATION) << "Detected min latency=" << minLatency << "; id=" << locationIdWithMinLatency.getHashString();

        // check whether best location needs to be changed
        if (!bestLocation_.isValid())
        {
            bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
        }
        else // if best location is valid
        {
            if (!bestLocation_.isDetectedFromThisAppStart())
            {
                bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
            }
            else if (bestLocation_.isDetectedWithDisconnectedIps())
            {
                if (isAllNodesInDisconnectedState)
                {
                    // check ping time changed more than 10% compared to prev best location
                    if ((double)minLatency < ((double)prevBestLocationLatency * 0.9))
                    {
                        bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
                    }
                }
            }
            else
            {
                if (isAllNodesInDisconnectedState)
                {
                    bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
                }
            }
        }
    }

    // send the signal to the GUI only if the location has actually changed
    if (bestLocation_.isValid() && prevBestLocationId != bestLocation_.getId())
    {
        if (prevBestLocationId.isValid()) {
            qCDebug(LOG_BEST_LOCATION) << "prevBestLocationId=" << prevBestLocationId.getHashString() << "; prevBestLocationLatency=" << prevBestLocationLatency;
        }

        if (locationIdWithMinLatency.isValid()) {
            qCDebug(LOG_BEST_LOCATION) << "Detected min latency=" << minLatency << "; id=" << locationIdWithMinLatency.getHashString();
        }

        qCDebug(LOG_BEST_LOCATION) << "Best location changed to " << bestLocation_.getId().getHashString();
        Q_EMIT bestLocationUpdated(bestLocation_.getId().apiLocationToBestLocation());
    }
}

BestAndAllLocations ApiLocationsModel::generateLocationsUpdated()
{
    QSharedPointer <QVector<types::Location> > items(new QVector<types::Location>());

    BestAndAllLocations ball;
    bool isBestLocationValid = false;

    for (const apiinfo::Location &l : locations_)
    {
        types::Location item;
        item.id = LocationID::createTopApiLocationId(l.getId());
        item.name = l.getName();
        item.countryCode = l.getCountryCode();
        item.isPremiumOnly = l.isPremiumOnly();
        item.isNoP2P = l.getP2P() == 0;

        for (int i = 0; i < l.groupsCount(); ++i)
        {
            const apiinfo::Group group = l.getGroup(i);
            types::City city;
            city.id = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick());
            city.city = group.getCity();
            city.nick = group.getNick();
            city.isPro = group.isPro();
            city.pingTimeMs = pingStorage_.getNodeSpeed(group.getPingIp());
            city.isDisabled = group.isDisabled();
            city.is10Gbps = (group.getLinkSpeed() == 10000);
            city.health = group.getHealth();
            item.cities << city;

            if (!isBestLocationValid && bestLocation_.isValid() && bestLocation_.getId() == city.id && !city.isDisabled)
            {
                isBestLocationValid = true;
            }
        }

        *items << item;
    }

    LocationID bestLocation;
    if (isBestLocationValid)
    {
        bestLocation = bestLocation_.getId().apiLocationToBestLocation();
    }
    // use first city of first location as best location (if at least on node in the city exists)
    else
    {
        for (int l = 0; l < items->count(); ++l)
        {
            for (int c = 0; c < items->at(l).cities.count(); ++c)
            {
                if (!items->at(l).cities[c].isDisabled)
                {
                    bestLocation = items->at(l).cities[c].id.apiLocationToBestLocation();
                    qCDebug(LOG_BEST_LOCATION) << "Best location chosen as the first city of the first location:" << items->at(l).cities[c].city << items->at(l).cities[c].nick;
                    break;
                }
            }
            if (bestLocation.isValid())
            {
                break;
            }
        }
    }

    // add static ips location
    if (staticIps_.getIpsCount() > 0)
    {
        types::Location item;

        item.id = LocationID::createTopStaticLocationId();
        item.name = QObject::tr("Static IPs");
        item.countryCode = "STATIC_IPS";
        item.isPremiumOnly = false;
        item.isNoP2P = false;
        ball.staticIpDeviceName = staticIps_.getDeviceName();

        for (int i = 0; i < staticIps_.getIpsCount(); ++i)
        {
            const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
            types::City city;
            city.id = LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp);
            city.city = sid.cityName;
            city.pingTimeMs = pingStorage_.getNodeSpeed(sid.getPingIp());
            city.isPro = true;
            city.isDisabled = false;
            city.staticIpCountryCode = sid.countryCode;
            city.staticIp = sid.staticIp;
            city.staticIpType = sid.type;

            item.cities << city;
        }

        *items << item;
    }

    ball.bestLocation = bestLocation;
    ball.locations = items;
    return ball;
}

void ApiLocationsModel::sendLocationsUpdated()
{
    BestAndAllLocations ball = generateLocationsUpdated();
    Q_EMIT locationsUpdated(ball.bestLocation, ball.staticIpDeviceName, ball.locations);
}

void ApiLocationsModel::whitelistIps()
{
    QStringList ips;
    for (const apiinfo::Location &l : locations_)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            ips << l.getGroup(i).getPingIp();
        }
    }
    ips << staticIps_.getAllPingIps();
    Q_EMIT whitelistIpsChanged(ips);
}

bool ApiLocationsModel::isChanged(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps)
{
    return locations_ != locations || staticIps_ != staticIps;
}


} //namespace locationsmodel
