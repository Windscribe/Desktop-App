#include "verticalscrollbarwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"


VerticalScrollBarWidget::VerticalScrollBarWidget(int width, int height, QWidget *parent) : QWidget(parent)
    , width_(width)
    , height_(height)
    , curOpacity_(OPACITY_HIDDEN)
    , curBackgroundColor_(Qt::white)
    , curForegroundColor_(Qt::white)
    , curBarPosY_(0)
    , curBarHeight_(height)
    , barPortion_(0)
    , stepSize_(1)
    , pressed_(false)
    , mouseOnClickY_(0)
{
    connect(&barPosYAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBarPosYChanged(QVariant)));
}

void VerticalScrollBarWidget::setHeight(int height)
{
    height_ = height;
    updateBarHeight();
}

void VerticalScrollBarWidget::setBarPortion(double portion)
{
    barPortion_ = portion;
    updateBarHeight();
}

void VerticalScrollBarWidget::setStepSizePercent(double stepSizePrecent)
{
    // TODO: synchronize step size of scroll bar with central widget
    stepSize_ = (stepSizePrecent * height_);
}

void VerticalScrollBarWidget::moveBarToPercentPos(double posPercentY)
{
    if (!pressed_) //  prevents concurrent updates while dragging
    {
        int newPos = static_cast<int>(-posPercentY * height_);

        // keep handle on the bar window
        if (newPos < 0) newPos = 0;
        if (newPos > height_ - curBarHeight_) newPos = height_ - curBarHeight_;

        if (curBarPosY_ != newPos)
        {
            curBarPosY_ = newPos;
            update();
        }
    }
}

void VerticalScrollBarWidget::setOpacity(double opacity)
{
    curOpacity_ = opacity;
    update();
}

void VerticalScrollBarWidget::setBackgroundColor(QColor color)
{
    curBackgroundColor_ = color;
    update();
}

void VerticalScrollBarWidget::setForegroundColor(QColor color)
{
    curForegroundColor_ = color;
    update();
}

void VerticalScrollBarWidget::onBarPosYChanged(const QVariant &value)
{
    curBarPosY_ = value.toInt();

    double percentPos = static_cast<double>( curBarPosY_) / height_;
    emit moved(percentPos);
    update();
}

void VerticalScrollBarWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // painter.fillRect(QRect(0,0, sizeHint().width(), sizeHint().height()), Qt::magenta);

    // draw background line
    painter.setOpacity(curOpacity_);
    QRectF bgRect(0, 1, width_*G_SCALE, height_);
    painter.fillRect(bgRect, curBackgroundColor_);

    // Foreground line
    painter.setOpacity(curOpacity_);
    QRectF fgRect(0 , (curBarPosY_+1), width_*G_SCALE, curBarHeight_);
    painter.fillRect(fgRect, curForegroundColor_);
}

void VerticalScrollBarWidget::mousePressEvent(QMouseEvent *event)
{
    int y = static_cast<int>( event->pos().y() );

    mouseOnClickY_ = y;

    if (inBarRegion(y))
    {
        pressed_ = true;
    }
    else // click outside handle
    {
        int diff = - height_/8;
        if (y > curBarPosY_) diff = height_/8;

        int newPos = diff + curBarPosY_;

        // keep bar in window
        if (newPos < 0) newPos = 0;
        if (newPos > height_ - curBarHeight_) newPos = height_ - curBarHeight_;

        startAnAnimation(barPosYAnimation_, curBarPosY_, newPos, ANIMATION_SPEED_FAST);
    }
}

void VerticalScrollBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    pressed_ = false;
}

void VerticalScrollBarWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed_)
    {
        int curY = static_cast<int>( event->pos().y() );
        int delta =  curY - mouseOnClickY_;
        int diff = mouseOnClickY_ - curBarPosY_;

        int newPos = 0;
        bool validDragIncrement = false;
        if (delta > 0) // dragging down
        {
            int i = curBarPosY_;
            while (i <= curY - diff) // must consume up to n thresholds
            {
                i += qCeil(stepSize_);
                newPos = i;
            }

            // always goes one too far
            newPos -= qCeil(stepSize_);
            if (newPos < curBarPosY_) newPos = curBarPosY_;

            // valid if greater than current value
            if (newPos > curBarPosY_) validDragIncrement = true;

        }
        else if (delta < 0) // dragging up
        {

            int i = curBarPosY_;
            while (i >= curY - diff) // must consume up to n thresholds
            {
                i -= qCeil(stepSize_);
                newPos = i;
            }

            // always goes one too far
            newPos += qCeil(stepSize_);
            if (newPos > curBarPosY_) newPos = curBarPosY_;

            // valid if less than current value
            if (newPos < curBarPosY_) validDragIncrement = true;
        }

        if (validDragIncrement)
        {
            // keep bar in window
            if (newPos < 0) newPos = 0;
            if (newPos > height_ - curBarHeight_) newPos = height_ - curBarHeight_;

            curBarPosY_ =  newPos;
            mouseOnClickY_ = curY;

            double percentPos = static_cast<double>( curBarPosY_ ) / height_;
            emit moved(percentPos);

            update();
        }
    }
}

void VerticalScrollBarWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    emit hoverEnter();
}

void VerticalScrollBarWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    emit hoverLeave();
}

bool VerticalScrollBarWidget::inBarRegion(int y)
{
    bool inside = false;
    if (y >= curBarPosY_ && y <= (curBarPosY_ + curBarHeight_))
    {
        inside = true;
    }
    return inside;
}

void VerticalScrollBarWidget::updateBarHeight()
{
    curBarHeight_ = static_cast<int>(qCeil(height_ * barPortion_));
}
