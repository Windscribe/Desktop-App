#include "sortedcitiesmodel.h"
#include "../locationsmodel_roles.h"

namespace gui {

SortedCitiesModel::SortedCitiesModel(QObject *parent) : QSortFilterProxyModel(parent),
    orderLocationsType_(ORDER_LOCATION_BY_GEOGRAPHY)
{
}

void SortedCitiesModel::setLocationOrder(ORDER_LOCATION_TYPE orderLocationType)
{
    if (orderLocationsType_ != orderLocationType)
    {
        orderLocationsType_ = orderLocationType;
        invalidate();
    }
}

bool SortedCitiesModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (orderLocationsType_ == ORDER_LOCATION_BY_GEOGRAPHY || orderLocationsType_ == ORDER_LOCATION_BY_ALPHABETICALLY)
    {
        return lessThanByAlphabetically(left, right);
    }
    else if (orderLocationsType_ == ORDER_LOCATION_BY_LATENCY)
    {
        return lessThanByLatency(left, right);
    }
    else
    {
        Q_ASSERT(false);
    }
    return true;

}

bool SortedCitiesModel::lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const
{
    return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
}

bool SortedCitiesModel::lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const
{
    int leftLatency = sourceModel()->data(left, PING_TIME).toInt();
    int rightLatency = sourceModel()->data(right, PING_TIME).toInt();
    if (leftLatency == rightLatency)
    {
        return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
    }
    else
    {
        if (leftLatency == -1)
        {
            return false;
        }
        else if (rightLatency == -1)
        {
            return true;
        }
        else
        {
            return leftLatency < rightLatency;
        }
    }
}

} //namespace gui
