#include "customconfigsmodel.h"
#include "types/locationid.h"
#include "../locationsmodel_roles.h"

namespace gui {

CustomConfgisModel::CustomConfgisModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool CustomConfgisModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex mi = sourceModel()->index(source_row, 0, source_parent);
    QVariant v = sourceModel()->data(mi, gui::LOCATION_ID);
    LocationID lid = qvariant_cast<LocationID>(v);
    return lid.isCustomConfigsLocation();
}

} //namespace gui
