#include "connectionbadgedots.h"

#include <QPainter>
#include "dpiscalemanager.h"

ConnectionBadgeDots::ConnectionBadgeDots(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent)
  , animationRunning_(false)
{
    circleOpacityAnimation_.setDuration(ANIMATION_DURATION);
    circleOpacityAnimation_.setStartValue(0.0);
    circleOpacityAnimation_.setEndValue(1.0);
    connect(&circleOpacityAnimation_, &QVariantAnimation::valueChanged, this, &ConnectionBadgeDots::onCircleOpacityAnimationChanged);
    connect(&circleOpacityAnimation_, &QVariantAnimation::finished, this, &ConnectionBadgeDots::onCircleOpacityAnimationFinished);
    initAnimation();
    recalcSize();
}

QRectF ConnectionBadgeDots::boundingRect() const
{
    return QRectF(0, 0, width_, HEIGHT * G_SCALE);
}

void ConnectionBadgeDots::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    double initOpacity = painter->opacity();
    QColor circleColor(255, 255, 255);
    painter->setBrush(circleColor);
    painter->setRenderHint(QPainter::Antialiasing);

    painter->setOpacity(circleOpacity1_ * initOpacity);
    QPointF c1(circleCenter1_, HEIGHT*G_SCALE/2);
    painter->drawEllipse(c1, RADIUS*G_SCALE, RADIUS*G_SCALE);

    painter->setOpacity(circleOpacity2_ * initOpacity);
    QPointF c2(circleCenter2_, HEIGHT*G_SCALE/2);
    painter->drawEllipse(c2, RADIUS*G_SCALE, RADIUS*G_SCALE);

    painter->setOpacity(circleOpacity3_ * initOpacity);
    QPointF c3(circleCenter3_, HEIGHT*G_SCALE/2);
    painter->drawEllipse(c3, RADIUS*G_SCALE, RADIUS*G_SCALE);
}

void ConnectionBadgeDots::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    recalcSize();
}

void ConnectionBadgeDots::start()
{
    animationRunning_ = true;
    circleOpacityAnimation_.start();
}

void ConnectionBadgeDots::stop()
{
    animationRunning_ = false;
    circleOpacityAnimation_.stop();
    initAnimation();
}

void ConnectionBadgeDots::initAnimation()
{
    circleOpacity1_ = CIRCLE_OPACITY_UPPER;
    circleOpacity2_ = CIRCLE_OPACITY_LOWER;
    circleOpacity3_ = CIRCLE_OPACITY_LOWER;
    circleOpacityAnimation_.setDirection(QAbstractAnimation::Forward);
}

void ConnectionBadgeDots::onCircleOpacityAnimationChanged(const QVariant &value)
{
    QString stage;
    double percent;
    if (value.toDouble() < .5)
    {
        stage = "(1)";
        percent = value.toDouble() / .5;
        circleOpacity1_ = CIRCLE_OPACITY_UPPER - percent*(CIRCLE_OPACITY_UPPER-CIRCLE_OPACITY_LOWER);
        circleOpacity2_ = CIRCLE_OPACITY_LOWER + percent*(CIRCLE_OPACITY_UPPER-CIRCLE_OPACITY_LOWER);
        circleOpacity3_ = CIRCLE_OPACITY_LOWER;
    }
    else
    {
        stage = "(2)";
        double offsetPercent = value.toDouble() - .5;
        percent = offsetPercent / .5;
        circleOpacity1_ = CIRCLE_OPACITY_LOWER;
        circleOpacity2_ = CIRCLE_OPACITY_UPPER - percent*(CIRCLE_OPACITY_UPPER-CIRCLE_OPACITY_LOWER);
        circleOpacity3_ = CIRCLE_OPACITY_LOWER + percent*(CIRCLE_OPACITY_UPPER-CIRCLE_OPACITY_LOWER);
    }

    // qDebug() << stage << " " << circleOpacity1_ << ", " << circleOpacity2_ << ", " << circleOpacity3_ << " - " << percent << "%";
    update();
}

void ConnectionBadgeDots::onCircleOpacityAnimationFinished()
{
    if (animationRunning_)
    {
        if (circleOpacityAnimation_.direction() == QAbstractAnimation::Forward)
        {
            //qDebug() << "Switching direction 1";
            circleOpacityAnimation_.setDirection(QAbstractAnimation::Backward);
        }
        else if (circleOpacityAnimation_.direction() == QAbstractAnimation::Backward)
        {
            //qDebug() << "Switching direction 2";
            circleOpacityAnimation_.setDirection(QAbstractAnimation::Forward);
        }
        circleOpacityAnimation_.start();
    }
}

void ConnectionBadgeDots::recalcSize()
{
    circleCenter1_ = MARGIN * G_SCALE + RADIUS * G_SCALE;
    circleCenter2_ = circleCenter1_ + 2*RADIUS*G_SCALE + SPACING*G_SCALE;
    circleCenter3_ = circleCenter2_ + 2*RADIUS*G_SCALE + SPACING*G_SCALE;
    width_ = circleCenter3_  + RADIUS*G_SCALE + MARGIN*G_SCALE;

}
