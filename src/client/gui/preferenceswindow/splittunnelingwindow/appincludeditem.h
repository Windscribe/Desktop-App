#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/togglebutton.h"
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
    bool isActive();

    void setSelected(bool selected) override;
    void updateScaling() override;

    void setClickable(bool clickable) override;

signals:
    void selectionChanged(bool selected);
    void deleteClicked();
    void activeChanged(bool active);

private slots:
    void onToggleChanged(bool checked);

private:
    types::SplitTunnelingApp app_;
    IconButton *deleteButton_;
    ToggleButton *toggleButton_;

    void updatePositions();
};

}
