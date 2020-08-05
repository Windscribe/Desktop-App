#include "customscrollbar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>

#include "graphicresources/fontmanager.h"

namespace GuiLocations {


CustomScrollBar::CustomScrollBar(QWidget *parent, bool bIsHidden)
    : QScrollBar(Qt::Vertical, parent), bIsHidden_(bIsHidden), isHover_(false),
      curOpacity_(UNHOVER_OPACITY), startOpacity_(0), endOpacity_(0)
{
    connect(&opacityTimer_, SIGNAL(timeout()), SLOT(onOpacityTimer()));
}

void CustomScrollBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // scrollbar background to prevent item bottom line bleedthrough
    painter.fillRect(QRect(0,0,geometry().width(), geometry().height()), FontManager::instance().getMidnightColor() );

    if (!bIsHidden_)
    {
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        painter.setOpacity(curOpacity_);

        // draw scrollbar body
        QRect rc = QRect(0, 0, geometry().width(), geometry().height());
        rc = rc.marginsAdded(QMargins(-2, -3, -4, -3));
        QBrush brushBody(QColor(76, 90, 100));
        painter.setBrush(brushBody);
        painter.setPen(Qt::transparent);
        painter.drawRoundedRect(rc, 1, 1);

        //draw slider
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        opt.subControls = QStyle::SC_All;
        rc = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
        rc = rc.marginsAdded(QMargins(-2, 0, -4, 0));
        QBrush brush(QColor(255, 255, 255));
        painter.setBrush(brush);
        painter.drawRoundedRect(rc, 1, 1);
    }
}

void CustomScrollBar::enterEvent(QEvent *event)
{
    if (bIsHidden_)
    {
        QScrollBar::enterEvent(event);
    }
    else
    {
        startOpacity_ = curOpacity_;
        endOpacity_ = HOVER_OPACITY;
        opacityElapsedTimer_.start();
        opacityTimer_.start(1);

        QScrollBar::enterEvent(event);
        update();
    }
}

void CustomScrollBar::leaveEvent(QEvent *event)
{
    if (bIsHidden_)
    {
        QScrollBar::leaveEvent(event);
    }
    else
    {
        startOpacity_ = curOpacity_;
        endOpacity_ = UNHOVER_OPACITY;
        opacityElapsedTimer_.start();
        opacityTimer_.start(1);

        QScrollBar::leaveEvent(event);
        update();
    }
}

void CustomScrollBar::onOpacityTimer()
{
    double d = (double)opacityElapsedTimer_.elapsed() / (double)OPACITY_DURATION;
    curOpacity_ = startOpacity_ + (endOpacity_ - startOpacity_) * d;
    if (d > 1.0)
    {
        curOpacity_ = endOpacity_;
        opacityTimer_.stop();
    }
    update();
}

} // namespace GuiLocations
