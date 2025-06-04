#pragma once

#include <QTextBrowser>
#include <QVariantAnimation>

#include "api_responses/notification.h"
#include "commongraphics/baseitem.h"
#include "graphicresources/independentpixmap.h"
#include "entryitem.h"
#include "messageitem.h"

namespace NewsFeedWindow {

class NewsEntryContainer : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    NewsEntryContainer(ScalableGraphicsObject *parent, int width);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setMessages(const QVector<api_responses::Notification> &arr, const QSet<qint64> &shownIds, int id);
    void updateRead();

signals:
    void messageRead(qint64 messageId);
    void heightChanged(int height);
    void scrollToPosition(int position);

private slots:
    void onEntryHeightChanged(int height);
    void onScrollToItem(EntryItem *item);

private:
    static constexpr int ROUNDED_RECT_RADIUS = 16;

    QVector<BaseItem *> items_;

    void updatePositions();
    void scrollToItem(EntryItem *item);
    void clearItems();
};

}
