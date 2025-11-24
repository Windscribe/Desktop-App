#include "newsfeedwindowitem.h"

#include <QPainter>
#include <QTimer>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "commongraphics/footerbackground.h"
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
    painter->fillRect(boundingRect().adjusted(0, 32*G_SCALE, 0, -9*G_SCALE), QBrush(QColor(9, 15, 25)));

    QRect rcCaption;
    // base background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 0, painter);
        rcCaption = QRect(52*G_SCALE, 0, 200*G_SCALE, 56*G_SCALE);
    } else {
        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 16*G_SCALE, painter);
        rcCaption = QRect(52*G_SCALE, 16*G_SCALE, 200*G_SCALE, 56*G_SCALE);
    }

    QSharedPointer<IndependentPixmap> pixmapBorder = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorder_);
    pixmapBorder->draw(0, 0, 350*G_SCALE, 32*G_SCALE, painter);

    QSharedPointer<IndependentPixmap> pixmapBorderExtension = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorderExtension_);
    pixmapBorderExtension->draw(0, 32*G_SCALE, 350*G_SCALE, getBottomResizeArea().toRect().top() - 32*G_SCALE, painter);

    // draw page caption
    painter->setPen(Qt::white);
    QFont font = FontManager::instance().getFont(16, QFont::Bold);
    painter->setFont(font);
    painter->drawText(rcCaption, Qt::AlignLeft | Qt::AlignVCenter, tr("News Feed"));

    // bottom-most background
    CommonGraphics::drawFooter(painter, getBottomResizeArea().toRect());
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

void NewsFeedWindowItem::onWindowExpanded()
{
    updateRead();
}

} // namespace NewsFeedWindow
