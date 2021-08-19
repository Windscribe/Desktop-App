#ifndef FIREWALLTURNOFFBUTTON_H
#define FIREWALLTURNOFFBUTTON_H

#include <QVariantAnimation>

#include "commongraphics/clickablegraphicsobject.h"
#include "graphicresources/fontdescr.h"

namespace LoginWindow {

class FirewallTurnOffButton : public ClickableGraphicsObject
{
    Q_OBJECT

public:
    explicit FirewallTurnOffButton(QString text, ScalableGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    void setActive(bool active);
    void animatedHide();

private slots:
    void onPositionChanged(QVariant position);

private:
    int width_;
    int height_;
    FontDescr font_descr_;
    QString text_;
    bool is_animating_;
    QVariantAnimation animation_;
};

}
#endif // FIREWALLTURNOFFBUTTON_H
