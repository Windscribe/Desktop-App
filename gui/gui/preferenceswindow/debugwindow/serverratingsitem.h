#ifndef SERVERRATINGSITEM_H
#define SERVERRATINGSITEM_H

#include "../baseitem.h"
#include "CommonGraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class ServerRatingsItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ServerRatingsItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void updateScaling();

signals:
    void clearButtonHoverEnter();
    void clearButtonHoverLeave();
    void clicked();

private:
    IconButton *btnClear_;
    DividerLine *line_;

    void updatePositions();
};

} // namespace PreferencesWindow

#endif // SERVERRATINGSITEM_H
