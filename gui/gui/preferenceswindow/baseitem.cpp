#include "baseitem.h"
#include "basepage.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

BaseItem::BaseItem(ScalableGraphicsObject *parent, int height, QString id, bool clickable) : ClickableGraphicsObject(parent),
    height_(height), id_(id)
{
    setClickable(clickable);
}

QRectF BaseItem::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, height_);
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

} // namespace PreferencesWindow
