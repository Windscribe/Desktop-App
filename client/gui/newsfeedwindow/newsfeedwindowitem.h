#pragma once

#include <QTextBrowser>
#include "../backend/backend.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/resizablewindow.h"
#include "newsfeedwindow/newscontentitem.h"

namespace NewsFeedWindow {

class NewsFeedWindowItem : public ResizableWindow 
{
    Q_OBJECT
public:
    explicit NewsFeedWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateRead();
    void setMessages(const QVector<types::Notification> &arr, const QSet<qint64> &shownIds);
    void setMessagesWithCurrentOverride(const QVector<types::Notification> &arr, const QSet<qint64> &shownIds, int overrideCurrentMessageId);

signals:
    void messageRead(qint64 messageId);

private:
    static constexpr int kMinHeight = 572;

    NewsContentItem *contentItem_;
};

}
