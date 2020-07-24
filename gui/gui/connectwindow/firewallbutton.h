#ifndef FIREWALLBUTTON_H
#define FIREWALLBUTTON_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "commongraphics/clickablegraphicsobject.h"

namespace ConnectWindow {

class FirewallButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit FirewallButton(ScalableGraphicsObject *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setFirewallAlwaysOn(bool isFirewallAlwaysOn);
    void setDisabled(bool isDisabled);


public slots:
    void setFirewallState(bool isFirewallEnabled);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);


private slots:
    void onAnimationValueChanged(const QVariant &value);

private:
    bool isFirewallEnabled_;
    bool isFirewallAlwaysOn_;
    bool isDisabled_;
    QVariantAnimation animation_;
};

} //namespace ConnectWindow

#endif // FIREWALLBUTTON_H
