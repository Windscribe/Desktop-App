#ifndef VIEWLOGITEM_H
#define VIEWLOGITEM_H

#include "../baseitem.h"
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class ViewLogItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ViewLogItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

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
