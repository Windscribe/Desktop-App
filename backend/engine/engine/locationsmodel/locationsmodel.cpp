#include "locationsmodel.h"

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent)
{

}

LocationsModel::~LocationsModel()
{

}

void LocationsModel::setLocations(const QVector<apiinfo::Location> &locations)
{
    QSharedPointer <QVector<LocationItem> > items(new QVector<LocationItem>());

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
            city.isDisabled = group.isDisabled();
            item.cities << city;
        }

        *items << item;
    }

    emit locationsUpdated(items);
}

void LocationsModel::setStaticIps(const apiinfo::StaticIps &staticIps)
{

}

void LocationsModel::clear()
{

}

} //namespace locationsmodel
