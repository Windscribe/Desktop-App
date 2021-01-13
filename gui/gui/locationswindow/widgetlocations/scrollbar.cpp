#include "scrollbar.h"

#include <QWheelEvent>
#include <QPainter>

#include <QDebug>

ScrollBar::ScrollBar(QWidget *parent) : QScrollBar(parent)
  , targetValue_(0)
{
    timer_.setInterval(1);
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimerTick()));
}

void ScrollBar::wheelEvent(QWheelEvent * event)
{
    // qDebug() << "delta: " << event->angleDelta();
    int stepVector;
    if (event->angleDelta().y() > 0)
    {
        // qDebug() << "Wheel Up";
        stepVector = -singleStep();
    }
    else
    {
        // qDebug() << "Wheel Down";
        stepVector = singleStep();
    }

    startValue_ = value();
    targetValue_ = targetValue_ + stepVector;
    if (targetValue_ > maximum()) targetValue_ = maximum();
    if (targetValue_ < minimum()) targetValue_ = minimum();
    // qDebug() << "Setting target: " << targetValue_;

    int diffPx = targetValue_ - startValue_;
    // TODO: optimize the animation speed
    animationDuration_ = qAbs(diffPx) * SCROLL_SPEED_FRACTION;
    // animationDuration_ = 1000; // for testing
    if (animationDuration_ < MINIMUM_DURATION) animationDuration_ = MINIMUM_DURATION;
    // qDebug() << "Animation duration: " << animationDuration_;

    elapsedTimer_.start();
    timer_.start();
}

void ScrollBar::paintEvent(QPaintEvent *event)
{
    // QPainter painter(this);
    // QRect bkgd(0,0,geometry().width(), geometry().height());
    // painter.fillRect(bkgd, Qt::gray);

    // qDebug() << "Painting scrollbar - max: " << maximum() << " pagestep: " << pageStep();
    QScrollBar::paintEvent(event);
}

void ScrollBar::onScrollAnimationValueChanged(const QVariant &value)
{
    setValue(value.toInt());
}

void ScrollBar::onTimerTick()
{
    double durationFraction = (double) elapsedTimer_.elapsed() / animationDuration_;
    if (durationFraction > 1)
    {
        // qDebug() << "Stopping timer";
        timer_.stop();
        setValue(targetValue_);
        return;
    }

    int curValue = startValue_ + durationFraction * (targetValue_ - startValue_);
    // qDebug() << "Setting value: " << curValue;
    setValue(curValue);
}
