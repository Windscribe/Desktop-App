#include "sortedlocationsmodel.h"
#include "../locationsmodel_roles.h"
#include "types/locationid.h"

namespace gui {

SortedLocationsModel::SortedLocationsModel(QObject *parent) : QSortFilterProxyModel(parent),
    orderLocationsType_(ORDER_LOCATION_BY_GEOGRAPHY)
{
}

void SortedLocationsModel::setLocationOrder(ORDER_LOCATION_TYPE orderLocationType)
{
    if (orderLocationsType_ != orderLocationType)
    {
        orderLocationsType_ = orderLocationType;
        invalidate();
    }
}

bool SortedLocationsModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
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

bool SortedLocationsModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, gui::LOCATION_ID);
    LocationID lid = qvariant_cast<LocationID>(v);
    return !lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation();
}

bool SortedLocationsModel::lessThanByGeography(const QModelIndex &left, const QModelIndex &right) const
{
    int leftInd = sourceModel()->data(left, INITITAL_INDEX).toInt();
    int rightInd = sourceModel()->data(right, INITITAL_INDEX).toInt();

    // the best location or custom config location  has indexes < 0, keep it at the beginning of the list
    if (leftInd < 0 || rightInd < 0)
    {
        return leftInd < rightInd;
    }

    // If it is a country then sort by index
    // the right index can not be checked, since it must correspond to the same type as the left one.
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, LOCATION_ID));
    if (leftLid.isTopLevelLocation())
    {
        return leftInd < rightInd;
    }
    // cities are sorted alphabetically
    else
    {
        return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
    }
}

bool SortedLocationsModel::lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const
{
    int leftInd = sourceModel()->data(left, INITITAL_INDEX).toInt();
    int rightInd = sourceModel()->data(right, INITITAL_INDEX).toInt();

    // the best location or custom config location  has indexes < 0, keep it at the beginning of the list
    if (leftInd < 0 || rightInd < 0)
    {
        return leftInd < rightInd;
    }

    return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
}

bool SortedLocationsModel::lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const
{
    int leftInd = sourceModel()->data(left, INITITAL_INDEX).toInt();
    int rightInd = sourceModel()->data(right, INITITAL_INDEX).toInt();

    // the best location or custom config location  has indexes < 0, keep it at the beginning of the list
    if (leftInd < 0 || rightInd < 0)
    {
        return leftInd < rightInd;
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
}

} //namespace gui

