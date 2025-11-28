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
        // Show if marked as favorite OR has a pinned IP
        bool isFavorite = sourceModel()->data(mi, kIsFavorite).toBool();
        QVariantList pinnedData = sourceModel()->data(mi, kPinnedIp).toList();
        bool hasPinnedIp = pinnedData.size() == 2 && (!pinnedData[0].toString().isEmpty() || !pinnedData[1].toString().isEmpty());
        bool isShowAsPremium = sourceModel()->data(mi, kIsShowAsPremium).toBool();

        return (isFavorite || hasPinnedIp) && !isShowAsPremium;
    }
}



} //namespace gui_locations
