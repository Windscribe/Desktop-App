#ifndef GUI_LOCATIONS_STATICIPSMODEL_H
#define GUI_LOCATIONS_STATICIPSMODEL_H

#include <QSortFilterProxyModel>

namespace gui_location {

class StaticIpsModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit StaticIpsModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

} //namespace gui_location


#endif // GUI_LOCATIONS_STATICIPSMODEL_H
