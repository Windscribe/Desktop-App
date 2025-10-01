#pragma once

#include <QSortFilterProxyModel>

namespace gui_locations {

class StaticIpsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit StaticIpsProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

} //namespace gui_locations

