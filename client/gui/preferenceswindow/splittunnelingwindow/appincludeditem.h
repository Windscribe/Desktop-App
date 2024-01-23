#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/iconbutton.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class AppIncludedItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit AppIncludedItem(types::SplitTunnelingApp app, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getName();
    QString getAppIcon();

    void setSelected(bool selected) override;
    void updateScaling() override;

    void setClickable(bool clickable) override;

signals:
    void selectionChanged(bool selected);
    void deleteClicked();

private:
    types::SplitTunnelingApp app_;
    IconButton *deleteButton_;

    void updatePositions();
};

}
