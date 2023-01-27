#ifndef LOCATIONITEMREGIONHEADERWIDGET_H
#define LOCATIONITEMREGIONHEADERWIDGET_H

#include <QLabel>
#include <QTextLayout>
#include "backend/locationsmodel/basiclocationsmodel.h"
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
    bool isBrokenConfig() const override;

    const LocationID getId() const override;
    const QString name() const override;
    ItemWidgetType type() override;

    bool containsCursor() const override;
    bool containsGlobalPoint(const QPoint &pt) override;
    QRect globalGeometry() const override;

    void setSelectable(bool selectable) override;
    void setAccented(bool accent) override;
    void setAccentedWithoutAnimation(bool accent) override;
    bool isAccented() const override;

    void setExpanded(bool expand);
    void setExpandedWithoutAnimation(bool expand);

    void updateScaling();

signals:
    void accented();
    void hoverEnter();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onOpacityAnimationValueChanged(const QVariant &value);
    void onExpandRotationAnimationValueChanged(const QVariant &value);

private:
    IWidgetLocationsInfo *widgetLocationsInfo_;
    LocationID locationID_;
    QString countryCode_;
    bool isPremiumOnly_;

    bool showPlusIcon_;
    bool accented_;
    bool selectable_;
    bool show10gbpsIcon_;
    int locationLoad_;

    // opacity animation
    double plusIconOpacity_;
    QVariantAnimation opacityAnimation_;

    // expand animation
    bool expanded_;
    double expandAnimationProgress_;
    QVariantAnimation expandAnimation_;

    // text
    double textOpacity_;
    QString textForLayout_;
    QSharedPointer<QTextLayout> textLayout_;
    QFont textLayoutFont();
    QRect textLayoutRect();
    void recreateTextLayout();

    // p2p
    bool showP2pIcon_;
    bool p2pHovering_;
    QRect p2pRect();
    void p2pIconHover();
    void p2pIconUnhover();
};

}
#endif // LOCATIONITEMREGIONHEADERWIDGET_H
