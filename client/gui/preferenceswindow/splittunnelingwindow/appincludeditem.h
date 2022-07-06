#ifndef APPINCLUDEDITEM_H
#define APPINCLUDEDITEM_H

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/iconbutton.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class AppIncludedItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit AppIncludedItem(types::SplitTunnelingApp app, QString iconPath, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    QString getName();
    QString getAppIcon();

    void setSelected(bool selected) override;
    void updateScaling() override;

    void setClickable(bool clickable) override;

signals:
    void selectionChanged(bool selected);
    void deleteClicked();

private:
    QString appIcon_;
    types::SplitTunnelingApp app_;
    IconButton *deleteButton_;

    void updatePositions();
};

}

#endif // APPINCLUDEDITEM_H
