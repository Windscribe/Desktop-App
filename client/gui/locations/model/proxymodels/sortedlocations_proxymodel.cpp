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

void SortedLocationsProxyModel::setFilter(const QString &filter)
{
    if (filter != filter_) {
        filter_ = filter;
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
        WS_ASSERT(false);
    }
    return true;
}

bool SortedLocationsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, kLocationId);
    LocationID lid = qvariant_cast<LocationID>(v);
    if (lid.isStaticIpsLocation() || lid.isCustomConfigsLocation())
        return false;

    if (filter_.isEmpty())
        return true;

    //  filtering by search string
    if (!source_parent.isValid()) {   // country
        QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
        if (mi.data().toString().contains(filter_, Qt::CaseInsensitive))
            return true;

        for (int i = 0, cnt = sourceModel()->rowCount(mi); i < cnt; ++i) {
            QModelIndex childMi = sourceModel()->index(i, 0, mi);
            if (childMi.data().toString().contains(filter_, Qt::CaseInsensitive))
                return true;
        }

    } else {    // city
        if (source_parent.data().toString().contains(filter_, Qt::CaseInsensitive))
            return true;

        QModelIndex childMi = sourceModel()->index(source_row, 0, source_parent);
        if (childMi.data().toString().contains(filter_, Qt::CaseInsensitive))
            return true;
    }

    return false;

}

bool SortedLocationsProxyModel::lessThanByGeography(const QModelIndex &left, const QModelIndex &right) const
{
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, kLocationId));
    LocationID rightLid = qvariant_cast<LocationID>(sourceModel()->data(right, kLocationId));

    // keep the best location on top
    if (leftLid.isBestLocation() && !rightLid.isBestLocation()) {
        return true;
    }

    if (!leftLid.isBestLocation() && rightLid.isBestLocation()) {
        return false;
    }

    // If it is a country then sort by index
    if (leftLid.isTopLevelLocation()) {
        return left.row() < right.row();
    }

    // cities are sorted alphabetically
    return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
}

bool SortedLocationsProxyModel::lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const
{
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, kLocationId));
    LocationID rightLid = qvariant_cast<LocationID>(sourceModel()->data(right, kLocationId));

    // keep the best location on top
    if (leftLid.isBestLocation() && !rightLid.isBestLocation()) {
        return true;
    }

    if (!leftLid.isBestLocation() && rightLid.isBestLocation()) {
        return false;
    }

    return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
}

bool SortedLocationsProxyModel::lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const
{
    LocationID leftLid = qvariant_cast<LocationID>(sourceModel()->data(left, kLocationId));
    LocationID rightLid = qvariant_cast<LocationID>(sourceModel()->data(right, kLocationId));

    // keep the best location on top
    if (leftLid.isBestLocation() && !rightLid.isBestLocation()) {
        return true;
    }

    if (!leftLid.isBestLocation() && rightLid.isBestLocation()) {
        return false;
    }

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
    return true;
}

} //namespace gui_locations

