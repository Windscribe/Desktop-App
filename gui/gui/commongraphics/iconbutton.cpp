#include "iconbutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QCursor>
#include "GraphicResources/imageresourcessvg.h"
#include "dpiscalemanager.h"

#include <QDebug>

IconButton::IconButton(int width, int height, const QString &imagePath, ScalableGraphicsObject *parent, double unhoverOpacity, double hoverOpacity) : ClickableGraphicsObject(parent),
  imagePath_(imagePath), width_(width), height_(height), curOpacity_(unhoverOpacity), unhoverOpacity_(unhoverOpacity), hoverOpacity_(hoverOpacity)
{
    connect(&imageOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onImageHoverOpacityChanged(QVariant)));
    connect(this, SIGNAL(hoverEnter()), SLOT(onHoverEnter()));
    connect(this, SIGNAL(hoverLeave()), SLOT(onHoverLeave()));

    setClickable(true);
}

QRectF IconButton::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_*G_SCALE);
}

void IconButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();
    painter->setOpacity(curOpacity_ * initialOpacity);

    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    int rcW = static_cast<int>(boundingRect().width() );
    int rcH = static_cast<int>(boundingRect().height());
    int w = static_cast<int>(p->width());
    int h = static_cast<int>(p->height());
    p->draw((rcW - w) / 2, (rcH - h) / 2, painter);
}

void IconButton::animateOpacityChange(double newOpacity, int animationSpeed)
{
    startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, newOpacity, animationSpeed);
}

void IconButton::setIcon(QString imagePath)
{
    imagePath_ = imagePath;
    prepareGeometryChange();
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    width_ = p->width()/G_SCALE;
    height_ = p->height()/G_SCALE;

    // qDebug() << "New Width: " << width_;
}

void IconButton::setSelected(bool selected)
{
    selected_ = selected;

    if (selected)
    {
        onHoverEnter();
    }
    else if (!stickySelection_)
    {
        onHoverLeave();
    }
}

void IconButton::setUnhoverOpacity(double unhoverOpacity)
{
    unhoverOpacity_ = unhoverOpacity;
}

void IconButton::setHoverOpacity(double hoverOpacity)
{
    hoverOpacity_ = hoverOpacity;
}

void IconButton::onHoverEnter()
{
    startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, hoverOpacity_, ANIMATION_SPEED_FAST);
}

void IconButton::onHoverLeave()
{
    if (!selected_)
    {
        startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, unhoverOpacity_, ANIMATION_SPEED_FAST);
    }
}

void IconButton::onImageHoverOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

