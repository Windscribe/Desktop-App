#include "sortedcities_proxymodel.h"
#include "../../locationsmodel_roles.h"
#include "types/locationid.h"

namespace gui_locations {

SortedCitiesProxyModel::SortedCitiesProxyModel(QObject *parent) : QSortFilterProxyModel(parent),
    orderLocationsType_(ORDER_LOCATION_BY_GEOGRAPHY)
{
}

void SortedCitiesProxyModel::setLocationOrder(ORDER_LOCATION_TYPE orderLocationType)
{
    if (orderLocationsType_ != orderLocationType)
    {
        orderLocationsType_ = orderLocationType;
        invalidate();
    }
}

bool SortedCitiesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
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
        WS_ASSERT(false);
    }
    return true;

}

bool SortedCitiesProxyModel::lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const
{
    return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
}

bool SortedCitiesProxyModel::lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const
{
    int leftLatency = sourceModel()->data(left, kPingTime).toInt();
    int rightLatency = sourceModel()->data(right, kPingTime).toInt();
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

} //namespace gui_locations
