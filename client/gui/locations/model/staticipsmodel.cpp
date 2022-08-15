#include "staticipsmodel.h"
#include "types/locationid.h"
#include "../locationsmodel_roles.h"

namespace gui_location {

StaticIpsModel::StaticIpsModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool StaticIpsModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, LOCATION_ID);
    LocationID lid = qvariant_cast<LocationID>(v);
    return lid.isStaticIpsLocation();
}

} //namespace gui_location
