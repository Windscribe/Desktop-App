#pragma once

#include <QSortFilterProxyModel>
#include "types/enums.h"

namespace gui_locations {

class SortedCitiesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortedCitiesProxyModel(QObject *parent = nullptr);

    void setLocationOrder(ORDER_LOCATION_TYPE orderLocationType);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    ORDER_LOCATION_TYPE orderLocationsType_;

    bool lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const;
    bool lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const;
};

} //namespace gui_locations

