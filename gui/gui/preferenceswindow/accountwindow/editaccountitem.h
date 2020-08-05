#ifndef EDITACCOUNTITEM_H
#define EDITACCOUNTITEM_H

#include "../baseitem.h"
#include "commongraphics/iconbutton.h"

namespace PreferencesWindow {

class EditAccountItem : public BaseItem
{
    Q_OBJECT
public:
    explicit EditAccountItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

private:
    IconButton *button_;

};

} // namespace PreferencesWindow

#endif // EDITACCOUNTITEM_H
