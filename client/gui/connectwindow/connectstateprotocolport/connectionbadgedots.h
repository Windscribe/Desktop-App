#pragma once

#include <QVariantAnimation>
#include "commongraphics/scalablegraphicsobject.h"

class ConnectionBadgeDots : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ConnectionBadgeDots(ScalableGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void start();
    void stop();

private slots:
    void onCircleOpacityAnimationChanged(const QVariant &value);
    void onCircleOpacityAnimationFinished();

private:
    // total of all margin, radius, spacing should = 36 (bg icon, unscaled)
    // 2 * margin, 6 * radius, 2 * spacing // 7,5,2 -> 2px rad //  6,3,3 -> 3px rad
    const int MARGIN = 6;
    const int SPACING = 3;
    const int RADIUS = 3;

    const int HEIGHT = 20;

    void recalcSize();

    // these are scaled
    int circleCenter1_;
    int circleCenter2_;
    int circleCenter3_;
    int width_;

    double circleOpacity1_;
    double circleOpacity2_;
    double circleOpacity3_;

    const double CIRCLE_OPACITY_LOWER = 0.2;
    const double CIRCLE_OPACITY_UPPER = 1.0;

    bool animationRunning_;
    const int ANIMATION_DURATION = 2000;
    QVariantAnimation circleOpacityAnimation_;
    void initAnimation();
};
