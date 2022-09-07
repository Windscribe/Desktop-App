#ifndef NEWSCONTENTITEM_H
#define NEWSCONTENTITEM_H

#include <QKeyEvent>
#include <QSet>
#include "entryitem.h"
#include "../backend/backend.h"
#include "commongraphics/basepage.h"

namespace NewsFeedWindow {

class NewsContentItem: public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit NewsContentItem(ScalableGraphicsObject *parent, int width);

    void setMessages(const QVector<types::Notification> &arr,
                     const QSet<qint64> &shownIds);
    void setMessagesWithCurrentOverride(const QVector<types::Notification> &arr,
                                        const QSet<qint64> &shownIds,
                                        int overrideCurrentMessageId);
    void updateRead();

public slots:
    void onScrollToItem(EntryItem *item);

signals:
    void messageRead(qint64 id);

private:
    static constexpr int SPACER_HEIGHT = 16;
    static constexpr int INDENT = 16;

    void setMessagesInternal(const QVector<types::Notification> &arr,
                             const QSet<qint64> &shownIds,
                             int id);
    void scrollToItem(EntryItem *item, bool expanded = false);
    EntryItem *findEntry(int id);

    int width_;
};

} // namespace NewsFeedWindow

#endif // NEWSCONTENTITEM_H
