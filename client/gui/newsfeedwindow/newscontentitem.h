#pragma once

#include <QSet>

#include "entryitem.h"
#include "commongraphics/basepage.h"

namespace NewsFeedWindow {

class NewsContentItem: public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit NewsContentItem(ScalableGraphicsObject *parent, int width);

    void setMessages(const QVector<api_responses::Notification> &arr,
                     const QSet<qint64> &shownIds);
    void setMessagesWithCurrentOverride(const QVector<api_responses::Notification> &arr,
                                        const QSet<qint64> &shownIds,
                                        int overrideCurrentMessageId);
    void updateRead();

public slots:
    void onScrollToItem(NewsFeedWindow::EntryItem *item);

signals:
    void messageRead(qint64 id);

private:
    static constexpr int SPACER_HEIGHT = 16;
    static constexpr int INDENT = 16;

    void setMessagesInternal(const QVector<api_responses::Notification> &arr,
                             const QSet<qint64> &shownIds,
                             int id);
    void scrollToItem(EntryItem *item, bool expanded = false);

    int width_;
};

} // namespace NewsFeedWindow
