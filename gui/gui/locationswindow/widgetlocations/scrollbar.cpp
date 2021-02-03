#include "scrollbar.h"

#include <QWheelEvent>
#include <QPainter>
#include <QStyleOption>
#include <qmath.h>
#include "dpiscalemanager.h"
#include "widgetlocationssizes.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"

#include <QDebug>

namespace GuiLocations {

ScrollBar::ScrollBar(QWidget *parent) : QScrollBar(parent)
  , targetValue_(0)
  , lastCursorPos_(0)
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
    rcHandle = rcHandle.marginsAdded(QMargins(-2*G_SCALE, 0, -4*G_SCALE, 0));
    QBrush brush(QColor(255, 255, 255));
    painter.setBrush(brush);
    painter.drawRoundedRect(rcHandle, 1, 1);

    // Note: some drawing handled via css (see customStyleSheet())
    QScrollBar::paintEvent(event);
}

void ScrollBar::forceSetValue(int val)
{
    // qDebug() << "Forcing: " << value() <<  " -> " << val;
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
                forceSetValue(proposedValue);
                emit handleDragged(proposedValue);
            }
        }
    }

    QScrollBar::mouseMoveEvent(event);
}

void ScrollBar::enterEvent(QEvent *event)
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
        // qDebug() << "Stopping timer";
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
                        .arg(qCeil(2))  // handle border-width
                        .arg(qCeil(2)); // handle border-radius
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
    return 3 * G_SCALE;
}

}
