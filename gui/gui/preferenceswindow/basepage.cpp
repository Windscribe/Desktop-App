#include "basepage.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

BasePage::BasePage(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    height_(0)
{

}

QRectF BasePage::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, height_);
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

void BasePage::addItem(BaseItem *item)
{
    item->setParentItem(this);
    connect(item, SIGNAL(heightChanged(int)), SLOT(recalcItemsPos()));
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

void BasePage::hideOpenPopups()
{
    Q_FOREACH(BaseItem *item, items_)
    {
        item->hideOpenPopups();
    }
}

void BasePage::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    recalcItemsPos();
}

QList<BaseItem *> BasePage::items() const
{
    return items_;
}

void BasePage::recalcItemsPos()
{
    int newHeight = 0;
    Q_FOREACH(BaseItem *item, items_)
    {
        item->setPos(0, newHeight);
        newHeight += item->boundingRect().height();
    }

    if (newHeight != height_)
    {
        prepareGeometryChange();
        height_ = newHeight;
        emit heightChanged(newHeight);
    }
}

} // namespace PreferencesWindow

