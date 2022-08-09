#ifndef GUI_LOCATIONS_SORTEDCITIESMODEL_H
#define GUI_LOCATIONS_SORTEDCITIESMODEL_H

#include <QSortFilterProxyModel>
#include "types/enums.h"

namespace gui {

class SortedCitiesModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortedCitiesModel(QObject *parent = nullptr);

    void setLocationOrder(ORDER_LOCATION_TYPE orderLocationType);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    ORDER_LOCATION_TYPE orderLocationsType_;

    bool lessThanByAlphabetically(const QModelIndex &left, const QModelIndex &right) const;
    bool lessThanByLatency(const QModelIndex &left, const QModelIndex &right) const;
};

} //namespace gui

#endif // GUI_LOCATIONS_SORTEDCITIESMODEL_H
