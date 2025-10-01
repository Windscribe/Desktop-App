#include "staticips_proxymodel.h"
#include "types/locationid.h"
#include "../../locationsmodel_roles.h"

namespace gui_locations {

StaticIpsProxyModel::StaticIpsProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool StaticIpsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, kLocationId);
    LocationID lid = qvariant_cast<LocationID>(v);
    return lid.isStaticIpsLocation();
}

} //namespace gui_locations
