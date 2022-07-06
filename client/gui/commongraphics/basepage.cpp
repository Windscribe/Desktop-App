#include "basepage.h"

namespace CommonGraphics {

BasePage::BasePage(ScalableGraphicsObject *parent, int width)
  : ScalableGraphicsObject(parent), height_(0), width_(width), spacerHeight_(0), indent_(0)
{
}

QRectF BasePage::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_);
}

void BasePage::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
}

int BasePage::currentPosY() const
{
    return static_cast<int>(y());
}

int BasePage::currentHeight() const
{
    return height_;
}

BaseItem *BasePage::item(int index)
{
    BaseItem *result = nullptr;

    if (index >= 0 && index < items_.length())
    {
        result = items_[index];
    }

    return result;
}

int BasePage::indexOf(BaseItem *item)
{
    return items_.indexOf(item);
}

void BasePage::addItem(BaseItem *item)
{
    item->setParentItem(this);
    connect(item, &BaseItem::heightChanged, this, &BasePage::recalcItemsPos);
    connect(item, &BaseItem::visibleChanged, this, &BasePage::recalcItemsPos);
    items_ << item;
    recalcItemsPos();
}

void BasePage::removeItem(BaseItem *itemToRemove)
{
    for (int index = 0; index < items_.length(); index++)
    {
        BaseItem *item = items_[index];

        if (item == itemToRemove)
        {
            item->disconnect();
            items_.removeAt(index);
            item->deleteLater();
            break;
        }
    }

    recalcItemsPos();
}

void BasePage::clearItems()
{
    for (auto item : items_)
    {
        item->disconnect();
        item->deleteLater();
    }
    items_.clear();

    recalcItemsPos();
}

void BasePage::setSpacerHeight(int height)
{
    spacerHeight_ = height;
    recalcItemsPos();
}

void BasePage::setIndent(int indent)
{
    indent_ = indent;
    recalcItemsPos();
}

void BasePage::hideOpenPopups()
{
    for (BaseItem *item : qAsConst(items_))
    {
        item->hideOpenPopups();
    }
}

void BasePage::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    recalcItemsPos();
}

bool BasePage::hasItemWithFocus()
{
    for (BaseItem *item : qAsConst(items_))
    {
        if (item->hasItemWithFocus())
        {
            return true;
        }
    }
    return false;
}

QList<BaseItem *> BasePage::items() const
{
    return items_;
}

void BasePage::recalcItemsPos()
{
    int newHeight = spacerHeight_*G_SCALE;
    for (BaseItem *item : qAsConst(items_))
    {
        if (!item->isVisible())
            continue;
        item->setPos(indent_*G_SCALE, newHeight);
        newHeight += item->boundingRect().height() + spacerHeight_*G_SCALE;
    }

    if (newHeight != height_)
    {
        prepareGeometryChange();
        height_ = newHeight;
        emit heightChanged(newHeight);
    }
}

} // namespace CommonGraphics