#ifndef LOCATIONITEMREGIONHEADERWIDGET_H
#define LOCATIONITEMREGIONHEADERWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "selectablelocationitemwidget.h"

namespace GuiLocations {

class LocationItemRegionHeaderWidget : public SelectableLocationItemWidget
{
    Q_OBJECT
public:
    explicit LocationItemRegionHeaderWidget(LocationModelItem *locationItem, QWidget *parent = nullptr);

    const QString name() const override;
    SelectableLocationItemWidgetType type() override;
    bool containsCursor() const override;
    void setSelected(bool select) override;
    bool isSelected() const override;

    LocationID getId();
    void updateScaling();
    static const int REGION_HEADER_HEIGHT = 50;

signals:
    void selected(SelectableLocationItemWidget *itemWidget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;

private:
    LocationID locationID_;
    QSharedPointer<QLabel> textLabel_;

    int height_;
    bool selected_;
    const QString labelStyleSheetWithOpacity(double opacity);

};

}
#endif // LOCATIONITEMREGIONHEADERWIDGET_H
