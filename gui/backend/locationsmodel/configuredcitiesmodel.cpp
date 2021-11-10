#include "configuredcitiesmodel.h"

ConfiguredCitiesModel::ConfiguredCitiesModel(QObject *parent) : BasicCitiesModel(parent)
{

}

void ConfiguredCitiesModel::update(QVector<QSharedPointer<LocationModelItem>> locations)
{
    clearCities();

    int cityId{};

    for (QSharedPointer<LocationModelItem> lmi : locations)
    {
        Q_ASSERT(lmi->id.isCustomConfigsLocation());
        for (int i = 0; i < lmi->cities.count(); ++i)
        {
            CityModelItem *cmi = new CityModelItem();
            *cmi = lmi->cities[i];
            cmi->initialInd_ = cityId++;
            cities_ << cmi;
        }
    }

    sort(cities_);
    emit itemsUpdated(cities_);
}

void ConfiguredCitiesModel::sort(QVector<CityModelItem *> &cities)
{
    // ORDER_LOCATION_BY_GEOGRAPHY ---> Sort by the order how cities came from engine.
    if (orderLocationsType_ == ProtoTypes::ORDER_LOCATION_BY_GEOGRAPHY)
    {
        std::sort(cities.begin(), cities.end(), [](const CityModelItem* item1, const CityModelItem* item2){
            return item1->initialInd_ < item2->initialInd_;
        });
    }
    else if (orderLocationsType_ == ProtoTypes::ORDER_LOCATION_BY_ALPHABETICALLY)
    {
        std::sort(cities.begin(), cities.end(), [](const CityModelItem* item1, const CityModelItem* item2){
            return item1->city.toLower() < item2->city.toLower();
        });
    }
    else if (orderLocationsType_ == ProtoTypes::ORDER_LOCATION_BY_LATENCY)
    {
        /// @todo Create global constants for 200 and 2000.
        auto calcPing = [](const CityModelItem* item) {
            if (item->pingTimeMs == PingTime::NO_PING_INFO) {
                return 200;
            }
            else if (item->pingTimeMs == PingTime::PING_FAILED) {
                return 2000;
            }
            else {
                return item->pingTimeMs.toInt();
            }
        };

        std::sort(cities.begin(), cities.end(), [&](const CityModelItem* item1, const CityModelItem* item2){
            return calcPing(item1) < calcPing(item2);
        });
    }
    else
    {
        Q_ASSERT(false);
    }
}
