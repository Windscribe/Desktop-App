#pragma once

#include <QTextBrowser>
#include "../backend/backend.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/resizablewindow.h"
#include "newsfeedwindow/newscontentitem.h"
#include "newsfeedwindow/inewsfeedwindow.h"

namespace NewsFeedWindow {

class NewsFeedWindowItem : public INewsFeedWindow
{
    Q_OBJECT
    Q_INTERFACES(INewsFeedWindow)
public:
    explicit NewsFeedWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void updateRead() override;
    void setMessages(const QVector<types::Notification> &arr,
                     const QSet<qint64> &shownIds) override;
    void setMessagesWithCurrentOverride(const QVector<types::Notification> &arr,
                                        const QSet<qint64> &shownIds,
                                        int overrideCurrentMessageId) override;

signals:
    void messageRead(qint64 messageId) override;

private:
    static constexpr int kMinHeight = 572;

    NewsContentItem *contentItem_;
};

}
