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
    if (orderLocationsType_ == ORDER_LOCATION_BY_GEOGRAPHY) {
        return lessThanByGeography(left, right);
    } else if (orderLocationsType_ == ORDER_LOCATION_BY_ALPHABETICALLY) {
        return lessThanByAlphabetically(left, right);
    } else if (orderLocationsType_ == ORDER_LOCATION_BY_LATENCY) {
        return lessThanByLatency(left, right);
    } else {
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
        if (mi.data().toString().contains(filter_, Qt::CaseInsensitive) ||
            mi.data(kCountryCode).toString().contains(filter_, Qt::CaseInsensitive))
        {
            return true;
        }

        for (int i = 0; i < sourceModel()->rowCount(mi); ++i) {
            QModelIndex childMi = sourceModel()->index(i, 0, mi);
            if (childMi.data().toString().contains(filter_, Qt::CaseInsensitive) ||
                childMi.data(kDisplayNickname).toString().contains(filter_, Qt::CaseInsensitive))
            {
                return true;
            }
        }

    } else {    // city
        if (source_parent.data().toString().contains(filter_, Qt::CaseInsensitive) ||
            source_parent.data(kCountryCode).toString().contains(filter_, Qt::CaseInsensitive))
        {
            return true;
        }

        QModelIndex childMi = sourceModel()->index(source_row, 0, source_parent);
        if (childMi.data().toString().contains(filter_, Qt::CaseInsensitive) ||
            childMi.data(kDisplayNickname).toString().contains(filter_, Qt::CaseInsensitive))
        {
            return true;
        }
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

    if (getSortGroup(left) != getSortGroup(right)) {
        return getSortGroup(left) < getSortGroup(right);
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

    if (getSortGroup(left) != getSortGroup(right)) {
        return getSortGroup(left) < getSortGroup(right);
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

    if (getSortGroup(left) != getSortGroup(right)) {
        return getSortGroup(left) < getSortGroup(right);
    }

    int leftLatency = sourceModel()->data(left, kPingTime).toInt();
    int rightLatency = sourceModel()->data(right, kPingTime).toInt();
    if (leftLatency == rightLatency) {
        return sourceModel()->data(left).toString() < sourceModel()->data(right).toString();
    } else {
        if (leftLatency == -1) {
            return false;
        } else if (rightLatency == -1) {
            return true;
        } else {
            return leftLatency < rightLatency;
        }
    }
    return true;
}

int SortedLocationsProxyModel::getSortGroup(const QModelIndex &index) const
{
    // If there's no search term, everything is in sort group 0
    if (filter_.isEmpty()) {
        return 0;
    }

    LocationID lid = qvariant_cast<LocationID>(sourceModel()->data(index, kLocationId));
    if (lid.isTopLevelLocation()) {
        // If it's a country and the country name or code starts with the filter, it goes first
        if (sourceModel()->data(index).toString().startsWith(filter_, Qt::CaseInsensitive) || sourceModel()->data(index, kCountryCode).toString().startsWith(filter_, Qt::CaseInsensitive)) {
            return 0;
        }

        // Otherwise, if any of its children starts with the filter, it goes next
        for (int i = 0; i < sourceModel()->rowCount(index); ++i) {
            QModelIndex childIndex = sourceModel()->index(i, 0, index);
            int childGroup = getSortGroup(childIndex);
            if (childGroup < 2) {
                return childGroup;
            }
        }
    } else if (sourceModel()->data(index, kDisplayNickname).toString().startsWith(filter_, Qt::CaseInsensitive) || sourceModel()->data(index).toString().startsWith(filter_, Qt::CaseInsensitive)) {
        return 1;
    }
    // Did not match any beginning of any filter, so it goes last
    return 2;
}

} //namespace gui_locations
