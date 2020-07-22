#ifndef EDITACCOUNTITEM_H
#define EDITACCOUNTITEM_H

#include "../baseitem.h"
#include "CommonGraphics/iconbutton.h"

namespace PreferencesWindow {

class EditAccountItem : public BaseItem
{
    Q_OBJECT
public:
    explicit EditAccountItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void updateScaling();

private:
    IconButton *button_;

};

} // namespace PreferencesWindow

#endif // EDITACCOUNTITEM_H
