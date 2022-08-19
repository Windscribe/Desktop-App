#pragma once

#include <QSortFilterProxyModel>
#include "types/enums.h"

namespace gui_locations {

// The model that sorts LocationsModel depending on the selected sorting algorithm
class SortedLocationsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortedLocationsProxyModel(QObject *parent = nullptr);
    void setLocationOrder(ORDER_LOCATION_TYPE orderLocationType);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    ORDER_LOCATION_TYPE orderLocationsType_;

    bool lessThanByGeography(const QModelIndex &left, const QModelIndex &right) const;
    bool lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const;
    bool lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const;
};

} //namespace gui_locations

