#include "scrollbar.h"

#include <QWheelEvent>
#include <QPainter>

#include <QDebug>

ScrollBar::ScrollBar(QWidget *parent) : QScrollBar(parent)
  , targetValue_(0)
  , lastCursorPos_(0)
  , pressed_(false)
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

    animateScroll(targetValue_ + stepVector, SCROLL_SPEED_FRACTION);
}

void ScrollBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // background
    // QRect bkgd(0,0,geometry().width(), geometry().height());
    // qDebug() << "Painting scrollbar: " << bkgd;
    // painter.fillRect(bkgd, Qt::red);

    QScrollBar::paintEvent(event);
}

void ScrollBar::forceSetValue(int value)
{
    setValue(value);
    targetValue_ = value;
}

void ScrollBar::mousePressEvent(QMouseEvent *event)
{
    bool mouseBelowHandlePos = event->pos().y() > value()*magicRatio();
    if (mouseBelowHandlePos && event->pos().y() < (value() + pageStep())*magicRatio()) // click in handle
    {
        // qDebug() << "Mouse press in handle";
        lastCursorPos_ = event->pos().y();
        lastValue_ = value();
        pressed_ = true;
    }
    else if (mouseBelowHandlePos)
    {
        // scroll down
        int proposedValue = value () + pageStep();
        if (proposedValue <= maximum())
        {
            forceSetValue(proposedValue);
        }
        else
        {
            forceSetValue(maximum());
        }
    }
    else
    {
        // scroll up
        int proposedValue = value() - pageStep();
        if (proposedValue >= 0)
        {
            forceSetValue(proposedValue);
        }
        else
        {
            forceSetValue(0);
        }
    }
}

void ScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    // qDebug() << "mouse release";
    pressed_ = false;
}

void ScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed_)
    {
        int diffPx = event->pos().y() - lastCursorPos_;
        int diffIncrements = diffPx/(singleStep()*magicRatio());

        int proposedValue = diffIncrements*singleStep() + lastValue_;
        if (proposedValue >= 0 && proposedValue <= maximum())
        {
            if (proposedValue != value())
            {
                // TODO: keep scroll dragging notching (no animation), but animate the view scrolling
                forceSetValue(proposedValue);
            }
        }
    }

    QScrollBar::mouseMoveEvent(event);
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

double ScrollBar::magicRatio() const
{
    return static_cast<double>(geometry().height())/(maximum()+pageStep());
}

void ScrollBar::animateScroll(int target, int animationSpeedFraction)
{
    startValue_ = value();
    targetValue_ = target;

    if (targetValue_ > maximum()) targetValue_ = maximum();
    if (targetValue_ < minimum()) targetValue_ = minimum();

    int diffPx = targetValue_ - startValue_;
    animationDuration_ = qAbs(diffPx) * animationSpeedFraction;
    if (animationDuration_ < MINIMUM_DURATION) animationDuration_ = MINIMUM_DURATION;

    elapsedTimer_.start();
    timer_.start();
}
