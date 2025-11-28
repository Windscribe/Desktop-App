#include "apilocationsmodel.h"

#include <QFile>
#include <QTextStream>

#include "mutablelocationinfo.h"
#include "nodeselectionalgorithm.h"

namespace locationsmodel {

ApiLocationsModel::ApiLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    pingManager_(this, stateController, networkDetectionManager, "pingStorage")
{
    if (bestLocation_.isValid())
    {
        qCDebug(LOG_BEST_LOCATION) << "Best location loaded from settings: " << bestLocation_.getId().getHashString();
    }
    else
    {
        qCDebug(LOG_BEST_LOCATION) << "No saved best location in settings";
    }
    connect(&pingManager_, &PingManager::pingInfoChanged, this, &ApiLocationsModel::onPingInfoChanged);
}

void ApiLocationsModel::setLocations(const QVector<api_responses::Location> &locations, const api_responses::StaticIps &staticIps)
{
    if (!isChanged(locations, staticIps)) {
        return;
    }

    QVector<api_responses::Location> prevLocations = locations_;
    locations_ = locations;
    staticIps_ = staticIps;

    whitelistIps();

    // ping stuff
    QVector<PingIpInfo> ips;
    for (const api_responses::Location &l : locations) {
        for (int i = 0; i < l.groupsCount(); ++i) {
            api_responses::Group group = l.getGroup(i);
            // Ping with Curl by hostname was introduced later, so the ping hostname may be empty when updating the program from an older version.
            if (!group.getPingHost().isEmpty()) {
                ips << PingIpInfo { group.getPingIp(), group.getPingHost(), group.getCity(), group.getNick(), wsnet::PingType::kHttp };
            }
        }
    }

    // handle static ips location
    for (int i = 0; i < staticIps_.getIpsCount(); ++i) {
        const api_responses::StaticIpDescr &sid = staticIps_.getIp(i);
        if (!sid.getPingHost().isEmpty()) {
            ips << PingIpInfo { sid.getPingIp(), sid.getPingHost(), sid.name, "staticIP", wsnet::PingType::kHttp };
        }
    }

    pingManager_.updateIps(ips);
    sendLocationsUpdated(prevLocations);
}

void ApiLocationsModel::clear()
{
    locations_.clear();
    staticIps_ = api_responses::StaticIps();
    pingManager_.clearIps();
    QSharedPointer<QVector<types::Location> > empty(new QVector<types::Location>());
    emit locationsUpdated(LocationID(), QString(),  empty);
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
                const api_responses::StaticIpDescr &sid = staticIps_.getIp(i);
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

    for (const api_responses::Location &l : locations_)
    {
        if (LocationID::createTopApiLocationId(l.getId()) == modifiedLocationId.toTopLevelLocation())
        {
            for (int i = 0; i < l.groupsCount(); ++i)
            {
                const api_responses::Group group = l.getGroup(i);
                if (LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()) == modifiedLocationId)
                {
                    QVector< QSharedPointer<const BaseNode> > nodes;
                    for (int n = 0; n < group.getNodesCount(); ++n)
                    {
                        const api_responses::Node &apiInfoNode = group.getNode(n);
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

void ApiLocationsModel::onPingInfoChanged(const QString &ip, int timems)
{
    if (pingManager_.isAllNodesHaveCurIteration()) {
        detectBestLocation(true);
    }

    for (const api_responses::Location &l : locations_) {
        for (int i = 0; i < l.groupsCount(); ++i) {
            const api_responses::Group group = l.getGroup(i);
            if (group.getPingIp() == ip) {
                emit locationPingTimeChanged(LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()), timems);
            }
        }
    }

    if (staticIps_.getIpsCount() > 0) {
        for (int i = 0; i < staticIps_.getIpsCount(); ++i) {
            const api_responses::StaticIpDescr &sid = staticIps_.getIp(i);
            if (sid.getPingIp() == ip) {
                emit locationPingTimeChanged(LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp), timems);
                break;
            }
        }
    }
}

void ApiLocationsModel::detectBestLocation(bool isAllNodesInDisconnectedState)
{
    int minLatency = INT_MAX;
    LocationID locationIdWithMinLatency;
    bool isPriorityBestLocation = false;

    // Commented debug entry out as this method is potentially called every minute and we don't
    // need to flood the log with this info.
    // qCDebug(LOG_BEST_LOCATION) << "LocationsModel::detectBestLocation, isAllNodesInDisconnectedState=" << isAllNodesInDisconnectedState;

    int prevBestLocationLatency = INT_MAX;

    // #1040 YOLO: try to find a best location that is 'priority' (10gbps, not disabled, and latency < 30ms) first
    for (const api_responses::Location &l : locations_) {
        for (int i = 0; i < l.groupsCount(); ++i) {
            const api_responses::Group group = l.getGroup(i);
            int latency = pingManager_.getPing(group.getPingIp()).toInt();

            if (group.isDisabled() || group.getLinkSpeed() < 10000 || latency == PingTime::NO_PING_INFO || latency == PingTime::PING_FAILED || latency > 30) {
                continue;
            }

            if (latency < minLatency) {
                minLatency = latency;
                locationIdWithMinLatency = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()) ;
                isPriorityBestLocation = true;
            }
        }
    }

    // If we didn't find a priority best location, then use the old logic
    if (!locationIdWithMinLatency.isValid()) {
        for (const api_responses::Location &l : locations_) {
            for (int i = 0; i < l.groupsCount(); ++i) {
                const api_responses::Group group = l.getGroup(i);

                if (group.isDisabled()) {
                    continue;
                }

                LocationID lid = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick());
                int latency = pingManager_.getPing(group.getPingIp()).toInt();

                // we assume a maximum ping time for three bars when no ping info
                if (latency == PingTime::NO_PING_INFO) {
                    latency = PingTime::LATENCY_STEP1;
                } else if (latency == PingTime::PING_FAILED) {
                    latency = PingTime::MAX_LATENCY_FOR_PING_FAILED;
                }

                if (bestLocation_.isValid() && lid == bestLocation_.getId()) {
                    prevBestLocationLatency = latency;
                }
                if (latency != PingTime::PING_FAILED && latency < minLatency) {
                    minLatency = latency;
                    locationIdWithMinLatency = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()) ;
                }
            }
        }
    }

    LocationID prevBestLocationId;
    if (bestLocation_.isValid()) {
        prevBestLocationId = bestLocation_.getId();
        // Commented debug entry out as this method is potentially called every minute and we don't
        // need to flood the log with this info.  We will log it if the location actually changes.
        //qCDebug(LOG_BEST_LOCATION) << "prevBestLocationId=" << prevBestLocationId.getHashString() << "; prevBestLocationLatency=" << prevBestLocationLatency;
    }


    if (locationIdWithMinLatency.isValid()) { // new best location found
        // Commented debug entry out as this method is potentially called every minute and we don't
        // need to flood the log with this info.  We will log it if the location actually changes.
        //qCDebug(LOG_BEST_LOCATION) << "Detected min latency=" << minLatency << "; id=" << locationIdWithMinLatency.getHashString();

        // check whether best location needs to be changed
        if (!bestLocation_.isValid()) {
            bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
        } else { // if best location is valid
            if (!bestLocation_.isDetectedFromThisAppStart()) {
                bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
            } else if (bestLocation_.isDetectedWithDisconnectedIps()) {
                if (isAllNodesInDisconnectedState) {
                    // check ping time changed more than 10% compared to prev best location, or we found a 10gbps location with latency < 30ms
                    if ((double)minLatency < ((double)prevBestLocationLatency * 0.9) || isPriorityBestLocation) {
                        bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
                    }
                }
            } else {
                if (isAllNodesInDisconnectedState) {
                    bestLocation_.set(locationIdWithMinLatency, true, isAllNodesInDisconnectedState);
                }
            }
        }
    }

    // send the signal to the GUI only if the location has actually changed
    if (bestLocation_.isValid() && prevBestLocationId != bestLocation_.getId()) {
        if (prevBestLocationId.isValid()) {
            qCDebug(LOG_BEST_LOCATION) << "prevBestLocationId=" << prevBestLocationId.getHashString() << "; prevBestLocationLatency=" << prevBestLocationLatency;
        }

        if (locationIdWithMinLatency.isValid()) {
            qCDebug(LOG_BEST_LOCATION) << "Detected min latency=" << minLatency << "; id=" << locationIdWithMinLatency.getHashString();
        }

        qCDebug(LOG_BEST_LOCATION) << "Best location changed to " << bestLocation_.getId().getHashString();
        emit bestLocationUpdated(bestLocation_.getId().apiLocationToBestLocation());
    }
}

PingTime ApiLocationsModel::getPrevPing(const QVector<api_responses::Location> &locations, int countryId, int cityId)
{
    for (const api_responses::Location &l : locations) {
        if (l.getId() != countryId) {
            continue;
        }
        for (int i = 0; i < l.groupsCount(); ++i) {
            if (l.getGroup(i).getId() == cityId) {
                return pingManager_.getPing(l.getGroup(i).getPingIp());
            }
        }
    }
    return PingTime::NO_PING_INFO;
}

BestAndAllLocations ApiLocationsModel::generateLocationsUpdated(const QVector<api_responses::Location> &prevLocations)
{
    QSharedPointer <QVector<types::Location> > items(new QVector<types::Location>());

    BestAndAllLocations ball;
    bool isBestLocationValid = false;

    for (const api_responses::Location &l : locations_)
    {
        types::Location item;
        item.id = LocationID::createTopApiLocationId(l.getId());
        item.idNum = l.getId();
        item.name = l.getName();
        item.countryCode = l.getCountryCode();
        item.shortName = l.getShortName();
        item.isPremiumOnly = l.isPremiumOnly();
        item.isNoP2P = l.getP2P() == 0;

        for (int i = 0; i < l.groupsCount(); ++i)
        {
            const api_responses::Group group = l.getGroup(i);
            types::City city;
            city.id = LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick());
            city.idNum = group.getId();
            city.city = group.getCity();
            city.nick = group.getNick();
            city.isPro = group.isPro();
            city.pingTimeMs = pingManager_.getPing(group.getPingIp());
            // If ping time does not exist, set it to previous ping, if it exists
            if (city.pingTimeMs == PingTime::NO_PING_INFO) {
                city.pingTimeMs = getPrevPing(prevLocations, item.idNum, city.idNum);
                pingManager_.setPing(group.getPingIp(), city.pingTimeMs);
            }
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
            const api_responses::StaticIpDescr &sid = staticIps_.getIp(i);
            types::City city;
            city.id = LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp);
            city.city = sid.cityName;
            city.pingTimeMs = pingManager_.getPing(sid.getPingIp());
            city.isPro = true;
            city.isDisabled = (sid.status != 1);
            city.staticIpCountryCode = sid.countryCode;
            city.staticIpShortName = sid.shortName;
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

void ApiLocationsModel::sendLocationsUpdated(const QVector<api_responses::Location> &prevLocations)
{
    BestAndAllLocations ball = generateLocationsUpdated(prevLocations);
    emit locationsUpdated(ball.bestLocation, ball.staticIpDeviceName, ball.locations);
}

void ApiLocationsModel::whitelistIps()
{
    QStringList ips;
    for (const api_responses::Location &l : locations_)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            ips << l.getGroup(i).getPingIp();
        }
    }
    ips << staticIps_.getAllPingIps();
    emit whitelistIpsChanged(ips);
}

bool ApiLocationsModel::isChanged(const QVector<api_responses::Location> &locations, const api_responses::StaticIps &staticIps)
{
    return locations_ != locations || staticIps_ != staticIps;
}


} //namespace locationsmodel
