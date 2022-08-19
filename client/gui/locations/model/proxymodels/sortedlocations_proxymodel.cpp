#include "sortedlocations_proxymodel.h"
#include "../../locationsmodel_roles.h"
#include "types/locationid.h"

namespace gui_locations {

SortedLocationsProxyModel::SortedLocationsProxyModel(QObject *parent) : QSortFilterProxyModel(parent),
    orderLocationsType_(ORDER_LOCATION_BY_GEOGRAPHY)
{
}

void SortedLocationsProxyModel::setLocationOrder(ORDER_LOCATION_TYPE orderLocationType)
{
    if (orderLocationsType_ != orderLocationType)
    {
        orderLocationsType_ = orderLocationType;
        invalidate();
    }
}

bool SortedLocationsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (orderLocationsType_ == ORDER_LOCATION_BY_GEOGRAPHY)
    {
        return lessThanByGeography(left, right);
    }
    else if (orderLocationsType_ == ORDER_LOCATION_BY_ALPHABETICALLY)
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

bool SortedLocationsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, LOCATION_ID);
    LocationID lid = qvariant_cast<LocationID>(v);
    return !lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation();
}

bool SortedLocationsProxyModel::lessThanByGeography(const QModelIndex &left, const QModelIndex &right) const
{
    // If it is a country then sort by index
    // the right index can not be checked, since it must correspond to the same type as the left one.
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, LOCATION_ID));
    if (leftLid.isTopLevelLocation())
    {
        return left.row() < right.row();
    }
    // cities are sorted alphabetically
    else
    {
        return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
    }
}

bool SortedLocationsProxyModel::lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const
{
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, LOCATION_ID));
    LocationID rightLid = qvariant_cast<LocationID>(sourceModel()->data(right, LOCATION_ID));

    // keep the best location on top
    if (leftLid.isBestLocation() && !rightLid.isBestLocation())
    {
        return true;
    }
    else if (!leftLid.isBestLocation() && rightLid.isBestLocation())
    {
        return false;
    }

    return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
}

bool SortedLocationsProxyModel::lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const
{
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, LOCATION_ID));
    LocationID rightLid = qvariant_cast<LocationID>(sourceModel()->data(right, LOCATION_ID));

    // keep the best location on top
    if (leftLid.isBestLocation() && !rightLid.isBestLocation())
    {
        return true;
    }
    else if (!leftLid.isBestLocation() && rightLid.isBestLocation())
    {
        return false;
    }

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
    return true;
}

} //namespace gui_locations

