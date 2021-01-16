#ifndef LOCATIONITEMREGIONHEADERWIDGET_H
#define LOCATIONITEMREGIONHEADERWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "selectablelocationitemwidget.h"
#include "commonwidgets/iconwidget.h"

namespace GuiLocations {

class LocationItemRegionHeaderWidget : public SelectableLocationItemWidget
{
    Q_OBJECT
public:
    explicit LocationItemRegionHeaderWidget(LocationModelItem *locationItem, QWidget *parent = nullptr);

    const LocationID getId() const override;
    const QString name() const override;
    SelectableLocationItemWidgetType type() override;
    bool containsCursor() const override;

    void setSelectable(bool selectable) override;
    void setSelected(bool select) override;
    bool isSelected() const override;

    void showPlusIcon(bool show);
    void setExpanded(bool expand);
signals:
    void selected(SelectableLocationItemWidget *itemWidget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    // void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onP2pIconHoverEnter();
    void onP2pIconHoverLeave();
    void onOpacityAnimationValueChanged(const QVariant &value);
    void onPlusRotationAnimationValueChanged(const QVariant &value);

private:
    LocationID locationID_;
    QString countryCode_;

    QSharedPointer<IconWidget> p2pIcon_;
    QSharedPointer<QLabel> textLabel_;

    bool showPlusIcon_;
    double plusIconOpacity_;
    double textOpacity_;
    QVariantAnimation opacityAnimation_;

    double expandAnimationProgress_;
    QVariantAnimation expandAnimation_;

    bool selected_;
    bool selectable_;
    const QString labelStyleSheetWithOpacity(double opacity);
    void recalcItemPositions();
};

}
#endif // LOCATIONITEMREGIONHEADERWIDGET_H
