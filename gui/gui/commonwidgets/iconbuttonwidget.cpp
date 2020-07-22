#include "iconbuttonwidget.h"

#include <QPainter>
#include "CommonGraphics/commongraphics.h"
#include "GraphicResources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace CommonWidgets {

IconButtonWidget::IconButtonWidget(QString imagePath, QWidget * parent) : QPushButton(parent),
    unhoverOpacity_(OPACITY_UNHOVER_ICON_STANDALONE),
    hoverOpacity_(OPACITY_FULL),
    curOpacity_(OPACITY_UNHOVER_ICON_STANDALONE)
{
    setImage(imagePath);

    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onOpacityChanged(QVariant)));

}

QSize IconButtonWidget::sizeHint() const
{
    return QSize(width_, height_);
}

void IconButtonWidget::setImage(QString imagePath)
{
    imagePath_ = imagePath;

    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath);

    if (p != nullptr)
    {
        width_ =  p->width();
        height_ = p->height();
    }

    update();
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
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    p->draw(0,0,&painter);
}

void IconButtonWidget::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, hoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::PointingHandCursor);
}

void IconButtonWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation(opacityAnimation_, curOpacity_, unhoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::ArrowCursor);
}

void IconButtonWidget::onOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

}
