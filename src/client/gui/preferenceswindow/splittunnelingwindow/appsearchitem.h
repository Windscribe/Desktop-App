#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class AppSearchItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    AppSearchItem(types::SplitTunnelingApp app, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getName();
    QString getFullName();
    QString getAppIcon();
    void setAppIcon(QString icon);

    void updateIcons();

    void setSelected(bool selected) override;
    void updateScaling() override;

private slots:
    void onOpacityChanged(const QVariant &value);
    void onHoverEnter();
    void onHoverLeave();

private:
    double opacity_;
    types::SplitTunnelingApp app_;

    QVariantAnimation opacityAnimation_;
};

}
