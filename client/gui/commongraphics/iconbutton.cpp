#include "iconbutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

#include <QDebug>

IconButton::IconButton(int width, int height, const QString &imagePath, const QString &shadowPath, ScalableGraphicsObject *parent, double unhoverOpacity, double hoverOpacity) : ClickableGraphicsObject(parent),
  imagePath_(imagePath), shadowPath_(shadowPath), width_(width), height_(height), curOpacity_(unhoverOpacity), unhoverOpacity_(unhoverOpacity), hoverOpacity_(hoverOpacity)
{
    if (!shadowPath_.isEmpty())
    {
        imageWithShadow_.reset(new ImageWithShadow(imagePath_, shadowPath_));
    }

    connect(&imageOpacityAnimation_, &QVariantAnimation::valueChanged, this, &IconButton::onImageHoverOpacityChanged);
    connect(this, &IconButton::hoverEnter, this, &IconButton::onHoverEnter);
    connect(this, &IconButton::hoverLeave, this, &IconButton::onHoverLeave);

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
    int rcW = static_cast<int>(boundingRect().width() );
    int rcH = static_cast<int>(boundingRect().height());

    if (imageWithShadow_)
    {
        int w = static_cast<int>(imageWithShadow_->width());
        int h = static_cast<int>(imageWithShadow_->height());
        imageWithShadow_->draw(painter, (rcW - w) / 2, (rcH - h) / 2);
    }
    else
    {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
        int w = static_cast<int>(p->width());
        int h = static_cast<int>(p->height());
        p->draw((rcW - w) / 2, (rcH - h) / 2, painter);
    }
}

void IconButton::animateOpacityChange(double newOpacity, int animationSpeed)
{
    startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, newOpacity, animationSpeed);
}

void IconButton::setIcon(const QString &imagePath)
{
    imagePath_ = imagePath;
    shadowPath_.clear();
    imageWithShadow_.reset();
    prepareGeometryChange();
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    width_ = p->width()/G_SCALE;
    height_ = p->height()/G_SCALE;

    // qDebug() << "New Width: " << width_;
}

void IconButton::setIcon(const QString &imagePath, const QString &shadowPath)
{
    imagePath_ = imagePath;
    shadowPath_ = shadowPath;
    imageWithShadow_.reset(new ImageWithShadow(imagePath_, shadowPath_));
    prepareGeometryChange();
    width_ = imageWithShadow_->width()/G_SCALE;
    height_ = imageWithShadow_->height()/G_SCALE;
}

void IconButton::setSelected(bool selected)
{
    selected_ = selected;
    hovered_ = selected;

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

void IconButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    if (!shadowPath_.isEmpty())
    {
        imageWithShadow_.reset(new ImageWithShadow(imagePath_, shadowPath_));
    }
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

