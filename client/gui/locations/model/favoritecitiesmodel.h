#ifndef GUI_LOCATIONS_FAVORITECITIESMODEL_H
#define GUI_LOCATIONS_FAVORITECITIESMODEL_H

#include <QSortFilterProxyModel>

namespace gui_location {

// The model which shows only the favorite locations
class FavoriteCitiesModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FavoriteCitiesModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

} //namespace gui_location

#endif // GUI_LOCATIONS_FAVORITECITIESMODEL_H
