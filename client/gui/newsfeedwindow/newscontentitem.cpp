#include "newscontentitem.h"

#include "utils/logger.h"
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

void NewsContentItem::setMessagesInternal(const QVector<types::Notification> &arr,
                                          const QSet<qint64> &shownIds,
                                          int id)
{
    bool seenUnread = false;
    clearItems();

    for (int i = 0; i < arr.size(); i++)
    {
        EntryItem *item = new EntryItem(this, arr[i], width_);
        connect(item, &EntryItem::messageRead, this, &NewsContentItem::messageRead);
        connect(item, &EntryItem::scrollToItem, this, &NewsContentItem::onScrollToItem);
        addItem(item);

        if (shownIds.contains(arr[i].id))
        {
            item->setRead(true);
        }

        // Expand the panel if there is only one item, if the specific id was requested, or if it's the first unread
        if ((arr.size() == 1) ||
            (id != -1 && arr[i].id == id) ||
            (!seenUnread && !shownIds.contains(arr[i].id)))
        {
            // don't mark it as read, yet
            item->setExpanded(true, false);
            seenUnread = true;
        }
    }
}

void NewsContentItem::updateRead()
{
    QList<CommonGraphics::BaseItem *> entries = items();

    for (int i = 0; i < entries.size(); i++)
    {
        EntryItem *entry = static_cast<EntryItem *>(entries[i]);
        if (entry->expanded())
        {
            scrollToItem(entry);
            entry->setRead(true);
        }
    }
}

void NewsContentItem::onScrollToItem(EntryItem *item)
{
    scrollToItem(item, true);
}

void NewsContentItem::scrollToItem(EntryItem *item, bool expanded)
{
    int delta = 0;
    if (expanded)
    {
        delta = item->expandedHeight() - item->collapsedHeight();
    }

    int scrollPos = item->y() + static_cast<CommonGraphics::ScrollArea *>(parentItem())->boundingRect().height() - TEXT_MARGIN*G_SCALE;
    if (scrollPos > currentHeight() + delta)
    {
        scrollPos = currentHeight() + delta;
    }
    emit scrollToPosition(scrollPos);
}

} // namespace NewsFeedWindow
