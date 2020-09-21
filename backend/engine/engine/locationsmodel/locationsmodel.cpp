#include "locationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager) : QObject(parent),
    pingIpsController_(this, stateController, networkStateManager)
{
    connect(&pingIpsController_, SIGNAL(pingInfoChanged(QString,int, bool)), SLOT(onPingInfoChanged(QString,int, bool)));
    connect(&pingIpsController_, SIGNAL(needIncrementPingIteration()), SLOT(onNeedIncrementPingIteration()));
    pingStorage_.incIteration();
}

LocationsModel::~LocationsModel()
{

}

void LocationsModel::setLocations(const QVector<apiinfo::Location> &locations)
{
    locations_ = locations;

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
    pingStorage_.updateNodes(stringListIps);
    pingIpsController_.updateIps(ips);


    generateLocationsUpdated();
}

void LocationsModel::setStaticIps(const apiinfo::StaticIps &staticIps)
{

}

void LocationsModel::clear()
{

}

void LocationsModel::setProxySettings(const ProxySettings &proxySettings)
{

}

void LocationsModel::disableProxy()
{

}

void LocationsModel::enableProxy()
{

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
}

void LocationsModel::onNeedIncrementPingIteration()
{
    pingStorage_.incIteration();
}

void LocationsModel::detectBestLocation(bool isAllNodesInDisconnectedState)
{
    int minLatency = INT_MAX;
    int min_ind = -1;
    int ind = 0;
    for (const apiinfo::Location &l : locations_)
    {
        int latency = calcLatency(l);
        if (latency != -1 && latency < minLatency)
        {
            minLatency = latency;
            min_ind = ind;
        }
        ind++;
    }

    if (min_ind != -1)
    {
        bestLocation_.setId(locations_[min_ind].getId());
    }
    generateLocationsUpdated();
}

void LocationsModel::generateLocationsUpdated()
{
    QSharedPointer <QVector<LocationItem> > items(new QVector<LocationItem>());

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

        if (bestLocation_.isValid() && bestLocation_.getId() == item.id)
        {
            item.id = LocationID::BEST_LOCATION_ID;
            item.name = "Best Location";
            items->insert(0,item);
        }
    }

    // use first location as best location
    if (!bestLocation_.isValid())
    {
        if (items->count() > 0)
        {
            LocationItem firstItem = items->at(0);
            firstItem.id = LocationID::BEST_LOCATION_ID;
            firstItem.name = "Best Location";
            items->insert(0,firstItem);
        }
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
