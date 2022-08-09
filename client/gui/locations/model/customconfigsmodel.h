#ifndef GUI_LOCATIONS_CUSTOMCONFIGSMODEL_H
#define GUI_LOCATIONS_CUSTOMCONFIGSMODEL_H

#include <QSortFilterProxyModel>

namespace gui {

class CustomConfgisModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit CustomConfgisModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

} //namespace gui


#endif // GUI_LOCATIONS_CUSTOMCONFIGSMODEL_H
