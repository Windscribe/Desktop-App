#include "newscontentitem.h"

#include <QPainter>

#include "dpiscalemanager.h"
#include "commongraphics/baseitem.h"
#include "commongraphics/scrollarea.h"
#include "entryitem.h"
#include "newsfeedconst.h"

namespace NewsFeedWindow {

NewsContentItem::NewsContentItem(ScalableGraphicsObject *parent, int width)
    : BasePage(parent, width), width_(width)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(SPACER_HEIGHT);
    setIndent(INDENT);
}

void NewsContentItem::setMessagesWithCurrentOverride(const QVector<types::Notification> &arr,
                                                     const QSet<qint64> &shownIds,
                                                     int overrideCurrentMessageId)
{
    setMessagesInternal(arr, shownIds, overrideCurrentMessageId);
}

void NewsContentItem::setMessages(const QVector<types::Notification> &arr,
                                  const QSet<qint64> &shownIds)
{
    setMessagesInternal(arr, shownIds, -1);
}

EntryItem *NewsContentItem::findEntry(int id)
{
    QList<CommonGraphics::BaseItem *> entries = items();
    for (int i = 0; i < entries.size(); i++)
    {
        EntryItem *entry = static_cast<EntryItem *>(entries[i]);
        if (id == entry->id())
        {
            return entry;
        }
    }
    return nullptr;
}

void NewsContentItem::setMessagesInternal(const QVector<types::Notification> &arr,
                                          const QSet<qint64> &shownIds,
                                          int id)
{
    bool seenUnread = false;
    QList<int> ids;

    for (int i = 0; i < arr.size(); i++)
    {
        ids << arr[i].id;

        EntryItem *entry = findEntry(arr[i].id);
        if (entry != nullptr)
        {
            entry->setItem(arr[i]);
        }
        else
        {
            entry = new EntryItem(this, arr[i], width_);
            connect(entry, &EntryItem::messageRead, this, &NewsContentItem::messageRead);
            connect(entry, &EntryItem::scrollToItem, this, &NewsContentItem::onScrollToItem);
            addItem(entry);
        }

        if (shownIds.contains(arr[i].id))
        {
            entry->setRead(true);
        }

        // Expand the panel if there is only one item, if the specific id was requested, or if it's the first unread
        if ((arr.size() == 1) ||
            (id != -1 && arr[i].id == id) ||
            (!seenUnread && !shownIds.contains(arr[i].id)))
        {
            // don't mark it as read, yet
            entry->setExpanded(true, false);
            seenUnread = true;
        }
    }

    // remove any items not in the ids list since they ahve been deleted.
    QList<CommonGraphics::BaseItem *> entries = items();
    for (int i = 0; i < entries.size(); i++)
    {
        EntryItem *entry = static_cast<EntryItem *>(entries[i]);
        if (!ids.contains(entry->id()))
        {
            removeItem(entry);
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

void NewsContentItem::onScrollToItem(EntryItem *item)
{
    scrollToItem(item, true);
}

void NewsContentItem::scrollToItem(EntryItem *item, bool expanded)
{
    emit scrollToPosition(item->y() - TEXT_MARGIN*G_SCALE);
}

} // namespace NewsFeedWindow
