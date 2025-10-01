#pragma once

#include <QSet>

#include "commongraphics/basepage.h"
#include "entryitem.h"
#include "newsentrycontainer.h"

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

signals:
    void messageRead(qint64 id);
    void scrollToPosition(int position);

private:
    static constexpr int SPACER_HEIGHT = 16;
    static constexpr int INDENT = 16;

    NewsEntryContainer *container_;

    int width_;
};

} // namespace NewsFeedWindow
