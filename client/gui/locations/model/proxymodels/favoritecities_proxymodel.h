#pragma once

#include <QSortFilterProxyModel>

namespace gui_locations {

// The model which shows only the favorite locations
class FavoriteCitiesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FavoriteCitiesProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

} //namespace gui_locations

