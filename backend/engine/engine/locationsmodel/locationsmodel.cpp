#include "locationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager) : QObject(parent),
    pingIpsController_(this, stateController, pingStorage_, networkStateManager)
{
}

LocationsModel::~LocationsModel()
{

}

void LocationsModel::setLocations(const QVector<apiinfo::Location> &locations)
{
    QSharedPointer <QVector<LocationItem> > items(new QVector<LocationItem>());

    bool bFirst = true;
    //QFile file("c:/ping/ping.txt");

    //file.open(QIODevice::WriteOnly);
    //QTextStream out(&file);

    for (const apiinfo::Location &l : locations)
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
            city.cityId = group.getCity();
            city.city = group.getCity();
            city.nick = group.getNick();
            city.isPro = group.isPro();
            city.pingTimeMs = PingTime(Utils::generateIntegerRandom(0, 2000));
            city.isDisabled = group.isDisabled();
            item.cities << city;

            //out << group.getPingIp() << "\n";
        }

        *items << item;

        if (bFirst)
        {
            bFirst = false;
            item.id = LocationID::BEST_LOCATION_ID;
            item.name = "Best Location";
            items->insert(0,item);
        }

    }

    //file.close();

    emit locationsUpdated(items);
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

} //namespace locationsmodel
