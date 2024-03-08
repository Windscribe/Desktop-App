#include "newsfeedwindowitem.h"

#include <QPainter>
#include <QTimer>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"

namespace NewsFeedWindow {

NewsFeedWindowItem::NewsFeedWindowItem(QGraphicsObject *parent,
                                       Preferences *preferences,
                                       PreferencesHelper *preferencesHelper)
    : ResizableWindow(parent, preferences, preferencesHelper)
{
    WS_ASSERT(preferencesHelper);
    setMinimumHeight(kMinHeight);

    contentItem_ = new NewsContentItem(this, WINDOW_WIDTH - 32);
    scrollAreaItem_->setItem(contentItem_);
    connect(contentItem_, &NewsContentItem::messageRead, this, &NewsFeedWindowItem::messageRead);
}

void NewsFeedWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // resize area background
    qreal initialOpacity = painter->opacity();
    painter->fillRect(boundingRect().adjusted(0, 286*G_SCALE, 0, -7*G_SCALE), QBrush(QColor(2, 13, 28)));

    QRect rcCaption;
    // base background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        QPainterPath path;
#ifdef Q_OS_MAC
        path.addRoundedRect(boundingRect().toRect(), 5*G_SCALE, 5*G_SCALE);
#else
        path.addRect(boundingRect().toRect());
#endif
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(2, 13, 28));
        painter->setPen(Qt::SolidLine);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 0, painter);
        rcCaption = QRect(64*G_SCALE, 2*G_SCALE, 200*G_SCALE, 56*G_SCALE);
    } else {
        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 27*G_SCALE, painter);
        rcCaption = QRect(64*G_SCALE, 30*G_SCALE, 200*G_SCALE, 56*G_SCALE);
    }

    // draw page caption
    painter->setPen(Qt::white);
    QFont *font = FontManager::instance().getFont(16, true);
    painter->setFont(*font);
    painter->drawText(rcCaption, Qt::AlignLeft | Qt::AlignVCenter, tr("News Feed"));

    // bottom-most background
    painter->setOpacity(initialOpacity);
    if (roundedFooter_) {
        painter->setPen(footerColor_);
        painter->setBrush(footerColor_);
        painter->drawRoundedRect(getBottomResizeArea(), 8*G_SCALE, 8*G_SCALE);
        painter->fillRect(getBottomResizeArea().adjusted(0, -2*G_SCALE, 0, -7*G_SCALE), QBrush(footerColor_));
    }
    else
    {
        painter->fillRect(getBottomResizeArea(), QBrush(footerColor_));
    }
}

void NewsFeedWindowItem::setMessagesWithCurrentOverride(const QVector<api_responses::Notification> &arr,
                                                        const QSet<qint64> &shownIds,
                                                        int overrideCurrentMessageId)
{
    contentItem_->setMessagesWithCurrentOverride(arr, shownIds, overrideCurrentMessageId);
}

void NewsFeedWindowItem::setMessages(const QVector<api_responses::Notification> &arr,
                                     const QSet<qint64> &shownIds)
{
    contentItem_->setMessages(arr, shownIds);
}

void NewsFeedWindowItem::updateRead()
{
    contentItem_->updateRead();
}

} // namespace NewsFeedWindow
