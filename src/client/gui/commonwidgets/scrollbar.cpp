#include "scrollbar.h"

#include <QWheelEvent>
#include <QPainter>
#include <QStyleOption>
#include <qmath.h>
#include <QDateTime>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"

namespace CommonWidgets {

ScrollBar::ScrollBar(QWidget *parent) : QScrollBar(parent)
  , targetValue_(0)
{
    scrollAnimation_.setEasingCurve(QEasingCurve::OutQuint);
    scrollAnimation_.setDuration(kScrollAnimationDiration);
    connect(&scrollAnimation_, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        this->setValue(value.toInt());
    });

    setStyleSheet(customStyleSheet());
    setAttribute(Qt::WA_TranslucentBackground);
}

void ScrollBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect rcHandle;
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;

    // Note: some drawing handled via css (see customStyleSheet())
    QScrollBar::paintEvent(event);
}

void ScrollBar::updateCustomStyleSheet()
{
    setStyleSheet(customStyleSheet());
    update();
}

void ScrollBar::setValueWithAnimation(int value)
{
    {
        const QSignalBlocker blocker(scrollAnimation_);
        scrollAnimation_.stop();
        scrollAnimation_.setStartValue((double)this->value());
        scrollAnimation_.setEndValue((double)value);
    }
    targetValue_ = value;
    this->setValue((double)this->value());      // This is necessary because of the peculiarities of the slider
    scrollAnimation_.start();
}

void ScrollBar::setValueWithoutAnimation(int value)
{
    scrollAnimation_.stop();
    targetValue_ = value;
    QScrollBar::setValue(value);
}

void ScrollBar::scrollDx(int delta)
{
    if (scrollAnimation_.state() == QAbstractAnimation::Running)
        targetValue_ += delta;
    else
        targetValue_ = this->value() + delta;

    targetValue_ = qMax(targetValue_, this->minimum());
    targetValue_ = qMin(targetValue_, this->maximum());
    setValueWithAnimation(targetValue_);
}

void ScrollBar::enterEvent(QEnterEvent *event)
{
    setCursor(Qt::PointingHandCursor);
    QScrollBar::enterEvent(event);
}

void ScrollBar::leaveEvent(QEvent *event)
{
    setCursor(Qt::ArrowCursor);
    QScrollBar::leaveEvent(event);
}

void ScrollBar::resizeEvent(QResizeEvent *event)
{
    QScrollBar::resizeEvent(event);
    updateCustomStyleSheet();
}

void ScrollBar::wheelEvent(QWheelEvent *event)
{
    // How did I figure out if event->phase() == Qt::NoScrollPhase then these are events from the mouse.
    // Otherwise the event from the touchpad - in this case, let's let it be processed in the QScrollBar.
    if (event->phase() == Qt::NoScrollPhase)
        scrollDx(-event->angleDelta().y());
    else
        QScrollBar::wheelEvent(event);
}

QString ScrollBar::customStyleSheet()
{
    // hover region is bigger than drawing region
    QString css = QString( "QScrollBar:vertical { margin: %1px %2 %3px %4px; ")
                    .arg(qCeil(0))   // top margin
                    .arg(qCeil(0))   // right
                    .arg(qCeil(0))   //  bottom margin
                    .arg(qCeil(0));  // left
    // body (visible region)
    css += QString("border: none; background-color: transparent; width: %1px; padding: %2 %3 %4 %5; }")
                    .arg(geometry().width())  // width
                    .arg(0)           // padding top
                    .arg(customPaddingWidth()) // padding right
                    .arg(0)           // padding bottom
                    .arg(0);          // padding left
    // handle - draw in paintEvent to change opacity
    css += QString( "QScrollBar::handle:vertical { background-color: %1; color: transparent;"
                    "border-color: rgb(83, 87, 94); border-width: %2px; border-style: solid; border-radius: %3px; min-height: %4px;}")
                        .arg(FontManager::instance().getMidnightColor().name())  // handle border-width
                        .arg(qCeil(1))  // handle border-width
                        .arg(qCeil((geometry().width() - customPaddingWidth())/2)) // handle border-radius
                        .arg(qCeil(50*G_SCALE)); // min-height
    css += QString( "QScrollBar::handle:vertical:hover { border-color: rgb(181, 183, 186); }");
    // top and bottom page buttons
    css += QString( "QScrollBar::add-line:vertical { border: none; background: none; }"
                    "QScrollBar::sub-line:vertical { border: none; background: none; }");
    css += QString( "QScrollBar::add-page:vertical { background-color: transparent; }");
    css += QString( "QScrollBar::sub-page:vertical { background-color: transparent; }");
    return css;
}

int ScrollBar::customPaddingWidth()
{
    return 5*G_SCALE;
}

} // namespace CommonWidgets
