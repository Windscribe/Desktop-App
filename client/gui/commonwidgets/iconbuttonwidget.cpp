#include "iconbuttonwidget.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"

namespace CommonWidgets {

IconButtonWidget::IconButtonWidget(QString imagePath, QWidget * parent) : QPushButton(parent)
  , unhoverOpacity_(OPACITY_UNHOVER_ICON_STANDALONE)
  , hoverOpacity_(OPACITY_FULL)
  , curOpacity_(OPACITY_UNHOVER_ICON_STANDALONE)
  , width_(0)
  , height_(0)
{
    setImage(imagePath);
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onOpacityChanged(QVariant)));
}

int IconButtonWidget::width()
{
    return width_;
}

int IconButtonWidget::height()
{
    return height_;
}

void IconButtonWidget::setImage(QString imagePath)
{
    imagePath_ = imagePath;
    updateSize();
}

void IconButtonWidget::setUnhoverHoverOpacity(double unhoverOpacity, double hoverOpacity)
{
    unhoverOpacity_ = unhoverOpacity;
    hoverOpacity_ = hoverOpacity;
}

void IconButtonWidget::animateOpacityChange(double targetOpacity)
{
    startAnAnimation<double>(opacityAnimation_, curOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void IconButtonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal initOpacity = painter.opacity();
    painter.setOpacity(curOpacity_ * initOpacity);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    p->draw(0,0,&painter);
}

void IconButtonWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, hoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::PointingHandCursor);
    emit hoverEnter();
}

void IconButtonWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, unhoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::ArrowCursor);
    emit hoverLeave();
}

void IconButtonWidget::onOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

void IconButtonWidget::updateSize()
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);

    if (p != nullptr)
    {
        int width =  p->width();
        int height = p->height();

        if (width != width_ || height != height_)
        {
            width_ = width;
            height_ = height;
            emit sizeChanged(width, height);
        }
    }
    update();
}

}
