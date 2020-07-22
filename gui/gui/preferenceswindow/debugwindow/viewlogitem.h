#ifndef VIEWLOGITEM_H
#define VIEWLOGITEM_H

#include "../baseitem.h"
#include "CommonGraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class ViewLogItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ViewLogItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void updateScaling();

signals:
    void viewButtonHoverEnter();
    void sendButtonHoverEnter();
    void buttonHoverLeave();

    void viewLogClicked();
    void sendLogClicked();

private:
    IconButton *btnSendLog_;
    IconButton *btnViewLog_;
    DividerLine *line_;

    void updatePositions();

};

} // namespace PreferencesWindow

#endif // VIEWLOGITEM_H
