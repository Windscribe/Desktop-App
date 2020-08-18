#ifndef SERVERRATINGSITEM_H
#define SERVERRATINGSITEM_H

#include "../baseitem.h"
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class ServerRatingsItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ServerRatingsItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

signals:
    void clearButtonHoverEnter();
    void clearButtonHoverLeave();

private:
    IconButton *btnClear_;
    DividerLine *line_;

    void updatePositions();
};

} // namespace PreferencesWindow

#endif // SERVERRATINGSITEM_H
