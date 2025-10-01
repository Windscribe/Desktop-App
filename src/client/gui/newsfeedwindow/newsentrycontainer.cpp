#include "newsentrycontainer.h"

#include "commongraphics/basepage.h"
#include "commongraphics/dividerline.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "newsfeedconst.h"

namespace NewsFeedWindow {

NewsEntryContainer::NewsEntryContainer(ScalableGraphicsObject *parent, int width)
    : BaseItem(parent, 0, "", false, width)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
}

void NewsEntryContainer::updateScaling()
{
    BaseItem::updateScaling();
}

void NewsEntryContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Rounded rectangle background
    painter->setOpacity(0.08);
    QPainterPath path;
    path.addRoundedRect(boundingRect(), ROUNDED_RECT_RADIUS*G_SCALE, ROUNDED_RECT_RADIUS*G_SCALE);
    painter->fillPath(path, QBrush(Qt::white));
}

void NewsEntryContainer::setMessages(const QVector<api_responses::Notification> &arr, const QSet<qint64> &shownIds, int id)
{
    // This list of EntryItem objects is only updated when the user opts to view
    // the notifications.  Thus, nuking the list and recreating it from the current
    // NotificationsController list is not that expensive, the logic is simple, and
    // ensures the entries are displayed from newest to oldest.  Notifications are
    // received from the server, and persisted locally by us, in that order.

    clearItems();

    bool seenUnread = false;
    for (const auto &notification : arr) {
        EntryItem *entry = new EntryItem(this, notification, width_);
        connect(entry, &EntryItem::messageRead, this, &NewsEntryContainer::messageRead);
        connect(entry, &EntryItem::scrollToItem, this, &NewsEntryContainer::onScrollToItem);
        connect(entry, &EntryItem::heightChanged, this, &NewsEntryContainer::onEntryHeightChanged);

        if (items_.size() > 0) {
            CommonGraphics::DividerLine *divider = new CommonGraphics::DividerLine(this, width_);
            items_ << divider;
        }
        items_ << entry;
        if (shownIds.contains(notification.id)) {
            entry->setRead(true);
        }

        // Expand the panel if there is only one item, if the specific id was requested, or if it's the first unread.
        if ((arr.size() == 1) ||
            (id != -1 && notification.id == id) ||
            (!seenUnread && !shownIds.contains(notification.id)))
        {
            // don't mark it as read, yet
            entry->setExpanded(true, false);
            seenUnread = true;
        }
    }

    updatePositions();
}

void NewsEntryContainer::updateRead()
{
    // find the first unread message, expand it without animation and scroll to it
    for (int i = 0; i < items_.size(); i++) {
        if (EntryItem *entry = dynamic_cast<EntryItem *>(items_[i])) {
            if (entry->isExpanded() && !entry->isRead()) {
                entry->setRead(true);
                scrollToItem(entry);
                return;
            }
        }
    }

    // if no such messages were found, scroll to the first expanded message
    for (auto item : items_) {
        if (EntryItem *entry = dynamic_cast<EntryItem *>(item)) {
            if (entry->isExpanded()) {
                scrollToItem(entry);
                return;
            }
        }
    }
}

void NewsEntryContainer::onScrollToItem(EntryItem *item)
{
    scrollToItem(item);
}

void NewsEntryContainer::scrollToItem(EntryItem *item)
{
    emit scrollToPosition(item->y() - TEXT_MARGIN*G_SCALE);
}

void NewsEntryContainer::updatePositions()
{
    int height = 0;

    for (auto item : items_) {
        item->setPos(0, height);
        height += item->boundingRect().height();
    }

    update();
    setHeight(height);
    update();

    emit heightChanged(height);
}

void NewsEntryContainer::clearItems()
{
    for (auto item : items_) {
        item->deleteLater();
    }
    updatePositions();
    items_.clear();
}

void NewsEntryContainer::onEntryHeightChanged(int height)
{
    updatePositions();
}

} // namespace NewsFeedWindow