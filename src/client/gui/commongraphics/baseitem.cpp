#include "baseitem.h"
#include "commongraphics.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

BaseItem::BaseItem(ScalableGraphicsObject *parent, int height, QString id, bool clickable, int width)
  : ClickableGraphicsObject(parent), height_(height), width_(width), id_(id), hideProgress_(1.0)
{
    setClickable(clickable);

    connect(&hideAnimation_, &QVariantAnimation::valueChanged, this, &BaseItem::onHideProgressChanged);
}

QRectF BaseItem::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_ * hideProgress_);
}

void BaseItem::setHeight(int height)
{
    if (height != height_)
    {
        prepareGeometryChange();
        height_ = height;
        emit heightChanged(height_);
    }
}

void BaseItem::hideOpenPopups()
{
    // overwrite
}

QString BaseItem::id()
{
    return id_;
}

void BaseItem::setId(QString id)
{
    id_ = id;
}

void BaseItem::hide(bool animate)
{
    // already hidden
    if (hideProgress_ == 0.0)
    {
        hideAnimation_.stop();
        emit hidden();
        return;
    }
    if (!animate)
    {
        hideProgress_ = 0.0;
        hideAnimation_.stop();
        onHideProgressChanged(0.0);
    }
    else
    {
        startAnAnimation(hideAnimation_, hideProgress_, 0.0, ANIMATION_SPEED_FAST);
    }
}

void BaseItem::show(bool animate)
{
    if (!animate)
    {
        hideProgress_ = 1;
        hideAnimation_.stop();
        onHideProgressChanged(1);
    }
    else
    {
        startAnAnimation(hideAnimation_, hideProgress_, 1.0, ANIMATION_SPEED_FAST);
    }
}

void BaseItem::onHideProgressChanged(const QVariant &value)
{
    prepareGeometryChange();
    hideProgress_ = value.toDouble();
    if (hideProgress_ == 0.0)
    {
        emit heightChanged(0);
        setVisible(false);
        emit hidden();
    }
    else
    {
        setVisible(true);
        emit heightChanged(height_ * hideProgress_);
    }
}

bool BaseItem::isHidden()
{
    return hideProgress_ != 1.0;
}

} // namespace CommonGraphics
