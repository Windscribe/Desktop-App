#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"

namespace PreferencesWindow {

class SplitTunnelingAppsItem: public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateScaling() override;

    void setClickable(bool clickable) override;

signals:
    void searchClicked();
    void addClicked();

private:
    void updatePositions();

    IconButton *searchButton_;
    IconButton *addButton_;
};

} // namespace PreferencesWindow
