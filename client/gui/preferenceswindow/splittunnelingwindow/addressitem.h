#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class AddressItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit AddressItem(types::SplitTunnelingNetworkRoute route, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getAddressText();
    types::SplitTunnelingNetworkRoute getNetworkRoute();

    void setSelected(bool selected) override;
    void updateScaling() override;

    void setClickable(bool clickable) override;

signals:
    void deleteClicked();

private slots:
    void onDeleteButtonHoverEnter();

private:
    types::SplitTunnelingNetworkRoute route_;
    QString text_;
    IconButton *deleteButton_;

    void updatePositions();
};

}
