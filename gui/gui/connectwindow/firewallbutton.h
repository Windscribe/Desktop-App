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

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setDisabled(bool isDisabled);

public slots:
    void setFirewallState(bool isFirewallEnabled);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;


private slots:
    void onAnimationValueChanged(const QVariant &value);

private:
    bool isFirewallEnabled_;
    bool isDisabled_;
    QVariantAnimation animation_;
};

} //namespace ConnectWindow

#endif // FIREWALLBUTTON_H
