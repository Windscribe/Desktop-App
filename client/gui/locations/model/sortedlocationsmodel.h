#ifndef GUI_LOCATIONS_SORTEDLOCATIONSMODEL_H
#define GUI_LOCATIONS_SORTEDLOCATIONSMODEL_H

#include <QSortFilterProxyModel>
#include "types/enums.h"

namespace gui {

// The model that sorts LocationsModel depending on the selected sorting algorithm
class SortedLocationsModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortedLocationsModel(QObject *parent = nullptr);
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

} //namespace gui

#endif // GUI_LOCATIONS_SORTEDLOCATIONSMODEL_H
