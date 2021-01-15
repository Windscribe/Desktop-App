#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "selectablelocationitemwidget.h"

namespace GuiLocations {

class LocationItemCityWidget : public SelectableLocationItemWidget
{
    Q_OBJECT
public:
    explicit LocationItemCityWidget(CityModelItem cityModelItem, QWidget *parent = nullptr);
    ~LocationItemCityWidget();

    const LocationID getId() const override;
    const QString name() const override;
    SelectableLocationItemWidgetType type() override;

    void setSelectable(bool selectable);
    void setSelected(bool select) override;
    bool isSelected() const override;
    bool containsCursor() const override;

    void setShowLatencyMs(bool showLatencyMs);

    static const int HEIGHT = 50;

signals:
    void selected(SelectableLocationItemWidget *itemWidget);
    void clicked(LocationItemCityWidget *itemWidget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QSharedPointer<QLabel> cityLabel_;
    QSharedPointer<QLabel> nickLabel_;
    CityModelItem cityModelItem_;

    bool selectable_;
    bool selected_;
    const QString labelStyleSheetWithOpacity(double opacity);

    void recalcItemPositions();

};

}

#endif // LOCATIONITEMCITYWIDGET_H

