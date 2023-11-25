#pragma once

#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include "commongraphics/clickablegraphicsobject.h"

namespace LoginWindow {

class IconHoverEngageButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit IconHoverEngageButton(const QString &imagePathDisabled, const QString &imagePathEnabled, ScalableGraphicsObject * parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

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

    static constexpr int WIDTH = 24;
    static constexpr int HEIGHT = 24;

    bool active_;

};

}
