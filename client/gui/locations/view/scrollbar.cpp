#include "scrollbar.h"

#include <QWheelEvent>
#include <QPainter>
#include <QStyleOption>
#include <qmath.h>
#include <QScroller>
#include <QDateTime>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"

namespace gui_locations {

ScrollBar::ScrollBar(QWidget *parent) : QScrollBar(parent)
  , targetValue_(0)
  , curOpacity_(OPACITY_HALF)
{
    anim_.setTargetObject(this);
    anim_.setPropertyName("value");
    anim_.setEasingCurve(QEasingCurve::OutQuint);
    anim_.setDuration(kScrollAnimationDiration);

    setStyleSheet(customStyleSheet());

    opacityAnimation_.setDuration(250);
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityAnimationValueChanged(QVariant)));
}

void ScrollBar::paintEvent(QPaintEvent *event)
{
    // background
    QPainter painter(this);
    painter.setOpacity(curOpacity_);
    QRect bkgd(0,0, geometry().width(), geometry().height());
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

void ScrollBar::updateCustomStyleSheet()
{
    setStyleSheet(customStyleSheet());
    update();
}

void ScrollBar::setValue(int value, bool bWithoutAnimation)
{
    if (bWithoutAnimation) {
        anim_.stop();
        QScrollBar::setValue(value);
    } else {
        anim_.stop();
        anim_.setStartValue((double)this->value());
        anim_.setEndValue((double)value);
        anim_.start();
    }
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

void ScrollBar::resizeEvent(QResizeEvent *event)
{
    QScrollBar::resizeEvent(event);
    updateCustomStyleSheet();
}

void ScrollBar::onOpacityAnimationValueChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
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
    css += QString("border: none; background: rgba(0,0,0,0); width: %1px; padding: %2 %3 %4 %5; }")
                    .arg(geometry().width())  // width
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

int ScrollBar::customPaddingWidth()
{
    return 2 * G_SCALE;
}

} // namespace gui_locations
