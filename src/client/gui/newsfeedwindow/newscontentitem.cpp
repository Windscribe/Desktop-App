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

    container_ = new NewsEntryContainer(this, width_);
    connect(container_, &NewsEntryContainer::scrollToPosition, this, &NewsContentItem::scrollToPosition);
    connect(container_, &NewsEntryContainer::messageRead, this, &NewsContentItem::messageRead);
    addItem(container_);
}

void NewsContentItem::setMessagesWithCurrentOverride(const QVector<api_responses::Notification> &arr,
                                                     const QSet<qint64> &shownIds,
                                                     int overrideCurrentMessageId)
{
    container_->setMessages(arr, shownIds, overrideCurrentMessageId);
}

void NewsContentItem::setMessages(const QVector<api_responses::Notification> &arr,
                                  const QSet<qint64> &shownIds)
{
    container_->setMessages(arr, shownIds, -1);
}

void NewsContentItem::updateRead()
{
    container_->updateRead();
}

} // namespace NewsFeedWindow
