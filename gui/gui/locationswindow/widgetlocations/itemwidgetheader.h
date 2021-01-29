#ifndef LOCATIONITEMREGIONHEADERWIDGET_H
#define LOCATIONITEMREGIONHEADERWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "iitemwidget.h"
#include "commonwidgets/iconwidget.h"
#include "iwidgetlocationsinfo.h"

namespace GuiLocations {

class ItemWidgetHeader : public IItemWidget
{
    Q_OBJECT
public:
    explicit ItemWidgetHeader(IWidgetLocationsInfo *widgetLocationsInfo, LocationModelItem *locationItem, QWidget *parent = nullptr);
    ~ItemWidgetHeader() override;

    bool isExpanded() const override;

    bool isForbidden() const override;
    bool isDisabled() const override;
    const LocationID getId() const override;
    const QString name() const override;
    ItemWidgetType type() override;

    bool containsCursor() const override;
    QRect globalGeometry() const override;


    void setSelectable(bool selectable) override;
    void setSelected(bool select) override;
    bool isSelected() const override;

    void setExpanded(bool expand);
    void setExpandedWithoutAnimation(bool expand);

signals:
    void selected();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onP2pIconHoverEnter();
    void onP2pIconHoverLeave();
    void onOpacityAnimationValueChanged(const QVariant &value);
    void onExpandRotationAnimationValueChanged(const QVariant &value);

private:
    LocationID locationID_;
    QString countryCode_;
    bool isPremiumOnly_;
    IWidgetLocationsInfo *widgetLocationsInfo_;

    QSharedPointer<IconWidget> p2pIcon_;
    QSharedPointer<QLabel> textLabel_;

    bool showPlusIcon_;
    double plusIconOpacity_;
    double textOpacity_;
    QVariantAnimation opacityAnimation_;

    double expandAnimationProgress_;
    QVariantAnimation expandAnimation_;

    bool expanded_;
    bool selected_;
    bool selectable_;
    const QString labelStyleSheetWithOpacity(double opacity);
    void recalcItemPositions();
};

}
#endif // LOCATIONITEMREGIONHEADERWIDGET_H
