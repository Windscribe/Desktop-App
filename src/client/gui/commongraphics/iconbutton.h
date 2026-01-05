#pragma once

#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include "commongraphics/commongraphics.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "utils/imagewithshadow.h"

class IconButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:

    explicit IconButton(int width, int height, const QString &imagePath, const QString &shadowPath, ScalableGraphicsObject * parent = nullptr, double unhoverOpacity = OPACITY_SIXTY, double hoverOpacity = OPACITY_FULL);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void animateOpacityChange(double newOpacity, int animationSpeed);

    void setIcon(const QString &imagePath, bool updateDimensions = true);
    void setIcon(const QString &imagePath, const QString &shadowPath, bool updateDimensions = true);

    void setSelected(bool selected) override;

    void setUnhoverOpacity(double unhoverOpacity);
    void setHoverOpacity(double hoverOpacity);
    void updateScaling() override;

    void setTintColor(const QColor &color);
    void setTooltip(const QString &tooltip);

    void hover();
    void unhover();

private slots:
    void onHoverEnter();
    void onHoverLeave();
    void onImageHoverOpacityChanged(const QVariant &value);

private:
    QString imagePath_;
    QString shadowPath_;
    QScopedPointer<ImageWithShadow> imageWithShadow_;

    int width_;
    int height_;

    double curOpacity_;
    QVariantAnimation imageOpacityAnimation_;

    double unhoverOpacity_;
    double hoverOpacity_;

    QString tooltip_;

    QColor tintColor_;
};
