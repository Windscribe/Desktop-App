#pragma once

#include <QVector>

#include "types/locationid.h"
#include "types/location.h"
#include "locationitem.h"

namespace gui_locations {

// utils functions used in the LocationsModel class
namespace utils
{
// vector with hash to speed up search in locations or city vector
template<typename Type>
class VectorWithHash : public QVector<Type>
{
public:
    VectorWithHash(const QVector<Type> &v) : QVector<Type>(v)
    {
        for (int i = 0; i < v.size(); i++) {
            map[v[i].id] = i;
        }
    }
    QHash<LocationID, int> map;
};

typedef VectorWithHash<types::Location> LocationsVector;
typedef VectorWithHash<types::City> CitiesVector;

QVector<int> findRemovedLocations(const QVector<gui_locations::LocationItem *> &original, const LocationsVector &changed);
QVector<int> findNewLocations(const QHash<LocationID, gui_locations::LocationItem *> &originalMapLocations, const LocationsVector &changed);
QVector<QPair<int, int> > findChangedLocations(const QVector<gui_locations::LocationItem *> &original, const LocationsVector &changed);
QVector<int> findMovedLocations(const QVector<gui_locations::LocationItem *> &original, const LocationsVector &changed, bool &outFound);
QVector<int> findRemovedCities(const QVector<types::City> &original, const CitiesVector &changed);
QVector<int> findNewCities(const QVector<types::City> &original, const CitiesVector &changed);
QVector<QPair<int, types::City> > findChangedCities(const QVector<types::City> &original, const CitiesVector &changed);
QVector<int> findMovedCities(const QVector<types::City> &original, const CitiesVector &changed, bool &outFound);

// to sort locations according to indexes
void sortLocations(QVector<LocationItem *> &locations, QVector<int> indexes);

} //namespace utils

} //namespace gui_locations
