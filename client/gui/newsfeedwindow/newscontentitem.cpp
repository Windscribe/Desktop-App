#include "newscontentitem.h"

#include "commongraphics/baseitem.h"
#include "newsfeedconst.h"

namespace NewsFeedWindow {

NewsContentItem::NewsContentItem(ScalableGraphicsObject *parent, int width)
    : BasePage(parent, width), width_(width)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(SPACER_HEIGHT);
    setIndent(INDENT);
}

void NewsContentItem::setMessagesWithCurrentOverride(const QVector<api_responses::Notification> &arr,
                                                     const QSet<qint64> &shownIds,
                                                     int overrideCurrentMessageId)
{
    setMessagesInternal(arr, shownIds, overrideCurrentMessageId);
}

void NewsContentItem::setMessages(const QVector<api_responses::Notification> &arr,
                                  const QSet<qint64> &shownIds)
{
    setMessagesInternal(arr, shownIds, -1);
}

void NewsContentItem::setMessagesInternal(const QVector<api_responses::Notification> &arr,
                                          const QSet<qint64> &shownIds,
                                          int id)
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
        connect(entry, &EntryItem::messageRead, this, &NewsContentItem::messageRead);
        connect(entry, &EntryItem::scrollToItem, this, &NewsContentItem::onScrollToItem);
        addItem(entry);

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
}

void NewsContentItem::updateRead()
{
    QList<CommonGraphics::BaseItem *> entries = items();

    // find the first unread message, expand it without animation and scroll to it
    for (int i = 0; i < entries.size(); i++)
    {
        EntryItem *entry = static_cast<EntryItem *>(entries[i]);
        if (entry->isExpanded() && !entry->isRead())
        {
            entry->setRead(true);
            scrollToItem(entry);
            return;
        }
    }

    // if no such messages were found, scroll to the first expanded message
    for (int i = 0; i < entries.size(); i++)
    {
        EntryItem *entry = static_cast<EntryItem *>(entries[i]);
        if (entry->isExpanded())
        {
            scrollToItem(entry);
            return;
        }
    }
}

void NewsContentItem::onScrollToItem(NewsFeedWindow::EntryItem *item)
{
    scrollToItem(item, true);
}

void NewsContentItem::scrollToItem(EntryItem *item, bool expanded)
{
    emit scrollToPosition(item->y() - TEXT_MARGIN*G_SCALE);
}

} // namespace NewsFeedWindow
