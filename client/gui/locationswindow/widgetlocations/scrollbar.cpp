#include "scrollbar.h"

#include <QWheelEvent>
#include <QPainter>
#include <QStyleOption>
#include <qmath.h>
#include "dpiscalemanager.h"
#include "widgetlocationssizes.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"

// #include <QDebug>

namespace GuiLocations {

ScrollBar::ScrollBar(QWidget *parent) : QScrollBar(parent)
  , targetValue_(0)
  , lastCursorPos_(0)
  , lastScrollDirectionUp_(false)
  , trackpadDeltaSum_(0)
  , trackPadScrollDelta_(0)
  , pressed_(false)
  , curOpacity_(OPACITY_HALF)
{
    scrollTimer_.setInterval(1);
    connect(&scrollTimer_, SIGNAL(timeout()), SLOT(onScollTimerTick()));
    setStyleSheet(customStyleSheet());

    opacityAnimation_.setDuration(250);
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityAnimationValueChanged(QVariant)));
}

void ScrollBar::wheelEvent(QWheelEvent * event)
{
//    qDebug() << event->source()
//             << ", wheel event delta: " << event->angleDelta()
//             << ", pixDelta: " << event->pixelDelta()
//             << ", delta: " << event->delta()
//             << ", globalPos: " << event->globalPosition() // cursor pos
//             << ", phase: " << event->phase();

	// Note: Windows10 trackpad does not appear to generate Synthesized system events so this block will only run on Mac
	// On Windows the trackpad event will run same block as mouse wheel (below)
    int stepVector = 0;
    if (event->source() == Qt::MouseEventSynthesizedBySystem) // touchpad scroll and flick
    {
        if (event->phase() == Qt::ScrollBegin)
        {
            trackPadScrollDelta_ = 0;
        }
        else if (event->phase() == Qt::ScrollUpdate) //  trackpad drag
        {
            trackPadScrollDelta_ += event->angleDelta().y();
            lastScrollDirectionUp_  = event->angleDelta().y() > 0;
        }
        else if (event->phase() == Qt::ScrollMomentum) // trackpad gesture/flick
        {
            trackPadScrollDelta_ += event->angleDelta().y();
            lastScrollDirectionUp_  = event->angleDelta().y() > 0;
        }
        else if (event->phase() == Qt::ScrollEnd) // remove finger from trackpad
        {
            if (trackPadScrollDelta_ == 0)
            {
                // we can stop the animation from proceeding, but can't update the scrollbar properly -- signal scrollarea for that
                scrollTimer_.stop();
                emit stopScroll(lastScrollDirectionUp_);
                return;
            }
        }

        if (trackPadScrollDelta_ > singleStep())
        {
            int diff = trackPadScrollDelta_ - singleStep();
            stepVector = -singleStep();
            trackPadScrollDelta_ = diff;
        }
        else if (trackPadScrollDelta_ < -singleStep())
        {
            int diff = trackPadScrollDelta_ + singleStep();
            stepVector = singleStep();
            trackPadScrollDelta_ = diff;
        }
    }
    else // mouse wheel or trackpad drag
    {

        // Some Windows trackpads send multple small deltas instead of one single large delta (like mouse scroll)
        // "Phase" data does not indicate anything helpful, all are "NoScrollPhase", unlike MacOS phase seen above
        // So we track scrolling until it accumulates to similar delta
        int change = event->angleDelta().y();
        lastScrollDirectionUp_  = event->angleDelta().y() > 0;
        if (abs(change) != 120 ) // must be trackpad
        {
            change += trackpadDeltaSum_;
        }
        else
        {
            // if someone starts using mouse (or +/- 120 trackpad) to scoll then clear accumulation
            trackpadDeltaSum_ = 0;
        }

        if (change > singleStep())
        {
            // qDebug() << "Wheel Up";
            stepVector = -singleStep();
            trackpadDeltaSum_ = change - singleStep();
        }
        else if (change < -singleStep()) // angle delta can be 0
        {
            // qDebug() << "Wheel Down";
            stepVector = singleStep();
            trackpadDeltaSum_ = change + singleStep();
        }
        else
        {
            trackpadDeltaSum_ = change;
        }
    }

    if (stepVector != 0)
    {
        // qDebug() << "Scrolling: " << targetValue_ + stepVector;
        animateScroll(targetValue_ + stepVector, SCROLL_SPEED_FRACTION);
    }
}

void ScrollBar::paintEvent(QPaintEvent *event)
{

    // background
    QPainter painter(this);
    painter.setOpacity(curOpacity_);
    QRect bkgd(0,0, geometry().width(), geometry().height());
    // qDebug() << "Painting scrollbar: " << bkgd;
    painter.fillRect(bkgd, FontManager::instance().getCarbonBlackColor());

    // background without padded region
    QRect thinBackground(0,0, geometry().width() - customPaddingWidth(), geometry().height());
    painter.fillRect(thinBackground, FontManager::instance().getScrollBarBackgroundColor());

    QRect rcHandle;
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    rcHandle = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    painter.fillRect(rcHandle, Qt::white);

    // Note: some drawing handled via css (see customStyleSheet())
    QScrollBar::paintEvent(event);
}

void ScrollBar::forceSetValue(int val)
{
    // qDebug() << "Forcing: " << value() <<  " -> " << val;
    scrollTimer_.stop();
    trackpadDeltaSum_= 0;
    targetValue_ = val;
    setValue(val);
}

bool ScrollBar::dragging()
{
    return pressed_;
}

void ScrollBar::updateCustomStyleSheet()
{
    setStyleSheet(customStyleSheet());
    update();
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

void ScrollBar::mouseReleaseEvent(QMouseEvent * /*event*/)
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
                forceSetValue(proposedValue);
                emit handleDragged(proposedValue);
            }
        }
    }

    QScrollBar::mouseMoveEvent(event);
}

void ScrollBar::enterEvent(QEnterEvent *event)
{
    setCursor(Qt::PointingHandCursor);
    opacityAnimation_.setStartValue(curOpacity_);
    opacityAnimation_.setEndValue(OPACITY_FULL);
    opacityAnimation_.start();
    QScrollBar::enterEvent(event);
}

void ScrollBar::leaveEvent(QEvent *event)
{
    setCursor(Qt::ArrowCursor);
    opacityAnimation_.setStartValue(curOpacity_);
    opacityAnimation_.setEndValue(OPACITY_HALF);
    opacityAnimation_.start();
    QScrollBar::leaveEvent(event);
}

void ScrollBar::onScrollAnimationValueChanged(const QVariant &value)
{
    setValue(value.toInt());
}

void ScrollBar::onScollTimerTick()
{
    double durationFraction = (double) scrollElapsedTimer_.elapsed() / animationDuration_;
    if (durationFraction > 1)
    {
        // qDebug() << "Stopping timer with target: " << targetValue_;
        scrollTimer_.stop();
        setValue(targetValue_);
        return;
    }

    int curValue = startValue_ + durationFraction * (targetValue_ - startValue_);
    // qDebug() << "Setting value: " << curValue;
    setValue(curValue);
}

void ScrollBar::onOpacityAnimationValueChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
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

    scrollElapsedTimer_.start();
    scrollTimer_.start();
}

const QString ScrollBar::customStyleSheet()
{

    // hover region is bigger than drawing region
    QString css = QString( "QScrollBar:vertical { margin: %1px %2 %3px %4px; ")
                    .arg(qCeil(0))   // top margin
                    .arg(qCeil(0))   // right
                    .arg(qCeil(0))   //  bottom margin
                    .arg(qCeil(0));  // left
    // body (visible region)
    css += QString("border: none; background: rgba(0,0,0,0); width: %1px; padding: %2 %3 %4 %5; }")
                    .arg(customScrollBarWidth() - customPaddingWidth())  // width
                    .arg(0)           // padding top
                    .arg(customPaddingWidth()) // padding right
                    .arg(0)           // padding bottom
                    .arg(0);          // padding left
    // handle - draw in paintEvent to change opacity
    css += QString( "QScrollBar::handle:vertical { background: rgba(0,0,0,0); color:  rgba(0,0,0,0);"
                    "border-width: %1px; border-style: solid; border-radius: %2px;}")
                        .arg(qCeil(0))  // handle border-width
                        .arg(qCeil(0)); // handle border-radius
    // top and bottom page buttons
    css += QString( "QScrollBar::add-line:vertical { border: none; background: none; }"
                     "QScrollBar::sub-line:vertical { border: none; background: none; }");
    return css;
}

int ScrollBar::customScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

int ScrollBar::customPaddingWidth()
{
    return 2 * G_SCALE;
}

}
