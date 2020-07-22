#ifndef ICONHOVERENGAGEBUTTON_H
#define ICONHOVERENGAGEBUTTON_H

#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include "CommonGraphics/clickablegraphicsobject.h"

namespace LoginWindow {

class IconHoverEngageButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit IconHoverEngageButton(const QString &imagePathDisabled, const QString &imagePathEnabled, ScalableGraphicsObject * parent = nullptr);
    virtual QRectF boundingRect() const;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setActive(bool active);
    bool active();

private slots:
    void onHoverEnter();
    void onHoverLeave();
    void onImageHoverOpacityChanged(const QVariant &value);

private:
    double curOpacity_;
    QVariantAnimation imageOpacityAnimation_;

    QString imagePath_;
    QString imagePathEnabled_;
    QString imagePathDisabled_;

    double unhoverOpacity();

    const int WIDTH = 24;
    const int HEIGHT = 24;

    bool active_;

};

}

#endif // ICONHOVERENGAGEBUTTON_H
