#include "favoritecities_proxymodel.h"
#include "../../locationsmodel_roles.h"
#include "types/locationid.h"

namespace gui_locations {

FavoriteCitiesProxyModel::FavoriteCitiesProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool FavoriteCitiesProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, kLocationId);
    LocationID lid = qvariant_cast<LocationID>(v);
    if (lid.isCustomConfigsLocation() || lid.isStaticIpsLocation())
    {
        return false;
    }
    else
    {
        return sourceModel()->data(mi, kIsFavorite).toBool() && !sourceModel()->data(mi, kIsShowAsPremium).toBool();
    }
}



} //namespace gui_locations
