#ifndef GUI_LOCATIONS_LOCATIONSMODEL_H
#define GUI_LOCATIONS_LOCATIONSMODEL_H

#include <QAbstractItemModel>
#include "favoritelocationsstorage.h"
#include "types/locationitem.h"
#include "types/locationid.h"
#include "types/pingtime.h"

namespace gui_location {

/*  It is an implementation of the locations model based on QAbstractItemModel
    The structure is a simple unsorted tree. Only the best location is always added to the top of the list,
    other items can be in any order, proxy models are used for sorting.

     BestLocation (if exists)
     CustomConfig Location  (if exists)
          custom_config1
          ...
          custom_configN
     Country1
          City1
          ...
          CityN
     Country2
          City1
          ...
          CityN
          ...
     CountryN

    Here also management of the best location and CustomConfig location.
    The best location has no children
    Roles for LocationsModel items are located in locationsmodel_roles.h
    Can be used with any Qt-class based on QAbstractItemView (for example with QTreeView) to show this model in the GUI.
*/

class LocationsModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    explicit LocationsModel(QObject *parent = nullptr);
    virtual ~LocationsModel();

    void updateLocations(const LocationID &bestLocation, const QVector<types::LocationItem> &locations);
    void updateBestLocation(const LocationID &bestLocation);
    void updateCustomConfigLocation(const types::LocationItem &location);
    void changeConnectionSpeed(LocationID id, PingTime speed);
    void setFreeSessionStatus(bool isFreeSessionStatus);

    int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	parent(const QModelIndex &index) const override;
    int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex getIndexByLocationId(const LocationID &id) const;
    QModelIndex getBestLocationIndex() const;

    // Could be region name, country code, server name
    // for example "Toronto", "The Six", "CA", "Canada East" would all be valid
    QModelIndex getIndexByFilter(const QString &strFilter) const;

private:

    class LocationWrapper
    {
    public:
        explicit LocationWrapper(const types::LocationItem &l, int ind) : location_(l), initialInd_(ind), is10gbps_(false)
        {
            load_ = calcLoad();
            recalcAveragePing();
        }

        // to add the best location
        explicit LocationWrapper(const LocationID &bestLocation, const types::LocationItem &l, int cityInd) : initialInd_(BEST_LOCATION_INTERNAL_IND)
        {
            types::CityItem city = l.cities[cityInd];

            location_.name = "Best Location";
            location_.id = bestLocation;
            location_.countryCode = l.countryCode;
            location_.isNoP2P = l.isNoP2P;
            location_.isPremiumOnly = l.isPremiumOnly;
            is10gbps_ = (city.linkSpeed == 10000);
            if (city.health >= 0 && city.health <= 100)
            {
                load_ = city.health;
            }
            else
            {
                load_ = 0;
            }
            averagePing_ = city.pingTimeMs.toInt();
            nickname_ = city.nick;
        }

        const types::LocationItem &location() const  {  return location_;  }
        int initialInd() const  {  return initialInd_;  }
        bool is10gbps() const { return is10gbps_;   }
        int load() const  {  return load_;  }
        int averagePing() const  {  return averagePing_;  }
        QString nickname() const { return nickname_; }
        void setPingTimeForCity(int cityInd, int time)
        {
            location_.cities[cityInd].pingTimeMs = time;
        }
        void setAveragePing(int time)
        {
            averagePing_ = time;
        }

        void recalcAveragePing()
        {
            // doesn't make sense for CustomConfig location
            if (location_.id.isCustomConfigsLocation())
            {
                averagePing_ = 0;
                return;
            }

            double sumPing = 0;
            int cnt = 0;
            for (int i = 0; i < location_.cities.size(); ++i)
            {
                int cityPing = location_.cities[i].pingTimeMs.toInt();
                if (cityPing == PingTime::NO_PING_INFO)
                {
                    sumPing += 200;     // we assume a maximum ping time for three bars
                }
                else if (cityPing == PingTime::PING_FAILED)
                {
                    sumPing += 2000;    // 2000 - max ping interval
                }
                else
                {
                    sumPing += cityPing;
                }
                cnt++;
            }

            if (cnt > 0)
            {
                averagePing_ = sumPing / (double)cnt;
            }
            else
            {
                averagePing_ = -1;
            }
        }

        inline bool operator==(const LocationWrapper& rhs)
        {
            if (initialInd_ != rhs.initialInd_ || is10gbps_ != rhs.is10gbps_ || load_ != rhs.load_ || averagePing_ != rhs.averagePing_ ||
                rhs.location_ != location_)
            {
                return false;
            }
            return true;
        }
        inline bool operator!=(const LocationWrapper& rhs) { return !(*this == rhs); }

        void updateCity(int cityInd, const types::CityItem &city)
        {
            Q_ASSERT(!location_.id.isBestLocation());
            location_.cities[cityInd] = city;
            load_ = calcLoad();
            recalcAveragePing();
        }

        void removeCity(int cityInd)
        {
            Q_ASSERT(!location_.id.isBestLocation());
            location_.cities.removeAt(cityInd);
            load_ = calcLoad();
            recalcAveragePing();
        }

        void addCity(const types::CityItem &city)
        {
            Q_ASSERT(!location_.id.isBestLocation());
            location_.cities << city;
            load_ = calcLoad();
            recalcAveragePing();
        }

        void updateLocationInfoOnly(const types::LocationItem &l, int ind)
        {
            initialInd_ = ind;
            location_.name = l.name;
            location_.countryCode = l.countryCode;
            location_.isPremiumOnly = l.isPremiumOnly;
            location_.isNoP2P = l.isNoP2P;
        }


    private:
        types::LocationItem location_;
        int initialInd_;
        bool is10gbps_;  // is10gbps makes sense only for the best location
        int load_;
        int averagePing_;
        QString nickname_;  // makes sense only for BestLocation

        // calc internal the load variable
        // Engine is using -1 to indicate to us that the load (health) value was invalid/missing,
        // and therefore this location should be excluded when calculating the region's average
        // load value.
        int calcLoad()
        {
            // doesn't make sense for CustomConfig location
            if (location_.id.isCustomConfigsLocation())
            {
                return 0;
            }

            qreal locationLoadSum = 0.0;
            int locationLoadCount = 0;
            for (int i = 0; i < location_.cities.size(); ++i)
            {
                int cityLoad = location_.cities[i].health;
                if (cityLoad >= 0 && cityLoad <= 100)
                {
                    locationLoadSum += cityLoad;
                    locationLoadCount += 1;
                }
            }

            if (locationLoadCount > 0)
            {
                return qRound(locationLoadSum / locationLoadCount);
            }
            else
            {
                return 0;
            }
        }
    };

    QVector<LocationWrapper *> locations_;
    QHash<LocationID, LocationWrapper *> mapLocations_;   // map LocationID to LocationWrapper* in locations_

    int *root_;   // Fake root node. The typename does not matter, only the pointer to identify the root node matters.
    bool isFreeSessionStatus_;

    FavoriteLocationsStorage favoriteLocationsStorage_;

    static constexpr int BEST_LOCATION_INTERNAL_IND = -2;
    static constexpr int CUSTOM_CONFIG_INTERNAL_IND = -1;

    QVariant dataForLocation(int row, int role) const;
    QVariant dataForCity(LocationWrapper *l, int row, int role) const;
    void clearLocations();
    LocationWrapper *findAndCreateBestLocation(const LocationID &bestLocation);

    void findCityChanges(const types::LocationItem &l1, const types::LocationItem &l2, QVector<int> &newCitiesInd, QVector<int> &removedCitiesInd,
                         QVector< QPair<int, int> > &changedCities);

};

} //namespace gui_location


#endif // GUI_LOCATIONS_LOCATIONSMODEL_H
