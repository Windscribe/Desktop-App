#include "locationsmodel_utils.h"

namespace gui_locations {
namespace utils {

QVector<int> findRemovedLocations(const QVector<gui_locations::LocationItem *> &original, const LocationsVector &changed)
{
    QVector<int> v;
    for (int ind = 0; ind < original.size(); ++ind)
    {
        LocationID lid = original[ind]->location().id;
        // skip the best location and custom configs location
        if (lid.isBestLocation() || lid.isCustomConfigsLocation())
        {
            continue;
        }

        if (!changed.map.contains(original[ind]->location().id))
        {
            v << ind;
        }
    }
    return v;
}

QVector<int> findNewLocations(const QHash<LocationID, gui_locations::LocationItem *> &originalMapLocations, const LocationsVector &changed)
{
    QVector<int> v;
    for (int ind = 0; ind < changed.size(); ++ind)
    {
        if (!originalMapLocations.contains(changed[ind].id))
        {
            v << ind;
        }
    }
    return v;
}

QVector<QPair<int, int> > findChangedLocations(const QVector<gui_locations::LocationItem *> &original, const LocationsVector &changed)
{
    QVector<QPair<int, int> > v;
    for (int ind = 0; ind < original.size(); ++ind)
    {
        LocationID lid = original[ind]->location().id;
        // skip the best location and custom configs location
        if (lid.isBestLocation() || lid.isCustomConfigsLocation())
        {
            continue;
        }

        auto it = changed.map.find(lid);
        if (it != changed.map.end())
        {
            if (original[ind]->location() != changed[it.value()])
            {
                v << qMakePair(ind, it.value());
            }
        }
        else
        {
            WS_ASSERT(false);
        }
    }
    return v;
}

QVector<int> findMovedLocations(const QVector<gui_locations::LocationItem *> &original, const LocationsVector &changed, bool &outFound)
{
    outFound = false;
    QVector<int> v;
    int offs = 0;   // to take into account the best location and custom configs location
    for (int ind = 0; ind < original.size(); ++ind)
    {
        LocationID lid = original[ind]->location().id;
        // skip the best location and custom configs location
        if (lid.isBestLocation() || lid.isCustomConfigsLocation())
        {
            v << ind;
            offs++;
            continue;
        }

        auto it = changed.map.find(lid);
        if (it != changed.map.end())
        {
            if (ind != (it.value() + offs))
            {
                outFound = true;
            }
            v << (it.value() + offs);
        }
        else
        {
            WS_ASSERT(false);
        }
    }

    return v;
}

QVector<int> findRemovedCities(const QVector<types::City> &original, const CitiesVector &changed)
{
    QVector<int> v;
    for (int ind = 0; ind < original.size(); ++ind)
    {
        if (!changed.map.contains(original[ind].id))
        {
            v << ind;
        }
    }
    return v;
}

QVector<int> findNewCities(const QVector<types::City> &original, const CitiesVector &changed)
{
    QVector<int> v;
    for (int ind = 0; ind < changed.size(); ++ind)
    {
        LocationID lid = changed[ind].id;
        if (std::find_if(original.begin(), original.end(),
            [&](const types::City &city) {
                return lid == city.id;
            }) == std::end(original))
        {
            v << ind;
        }
    }
    return v;
}

QVector<QPair<int, types::City> > findChangedCities(const QVector<types::City> &original, const CitiesVector &changed)
{
    WS_ASSERT(original.size() == changed.size());
    QVector<QPair<int, types::City> > v;
    for (int ind = 0; ind < original.size(); ++ind)
    {
        auto it = changed.map.find(original[ind].id);
        if (it != changed.map.end())
        {
            if (original[ind] != changed[it.value()])
            {
                v << qMakePair(ind, changed[it.value()]);
            }
        }
        else
        {
            WS_ASSERT(false);
        }
    }
    return v;
}

QVector<int> findMovedCities(const QVector<types::City> &original, const CitiesVector &changed, bool &outFound)
{
    WS_ASSERT(original.size() == changed.size());
    outFound = false;
    QVector<int> v;
    for (int ind = 0; ind < original.size(); ++ind)
    {
        auto it = changed.map.find(original[ind].id);
        if (it != changed.map.end())
        {
            if (ind != it.value())
            {
                outFound = true;
            }
            v << it.value();
        }
        else
        {
            WS_ASSERT(false);
        }
    }

    return v;
}

void sortLocations(QVector<LocationItem *> &locations, QVector<int> indexes)
{
    WS_ASSERT(locations.size() == indexes.size());
    // Selection sort algorithm
    for (int i = 0; i < indexes.size(); i++)
    {
        int minz = indexes[i];
        int ind = i;
        for (int j = i + 1; j < indexes.size(); j++)
        {
            if (indexes[j] < minz)
            {
                minz = indexes[j];
                ind = j;
            }
        }
        if (i != ind)
        {
            indexes.move(ind, i);
            locations.move(ind, i);
        }
    }
}

} //namespace utils
} //namespace gui_locations
