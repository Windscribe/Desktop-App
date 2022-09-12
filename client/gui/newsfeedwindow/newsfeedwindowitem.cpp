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
    : ScalableGraphicsObject(parent), preferences_(preferences), curScale_(1.0)
{
    WS_ASSERT(preferencesHelper);
    setFlags(QGraphicsObject::ItemIsFocusable);

    curHeight_ = (MIN_HEIGHT - 100);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    backgroundBase_ = "background/WIN_TOP_BG";
    backgroundHeader_ = "background/WIN_HEADER_BG_OVERLAY";
    roundedFooter_ = false;
#else
    backgroundBase_ = "background/MAC_TOP_BG";
    backgroundHeader_ = "background/MAC_HEADER_BG_OVERLAY";
    roundedFooter_ = true;
#endif
    footerColor_ = FontManager::instance().getCharcoalColor();

    connect(preferences, &Preferences::appSkinChanged, this, &NewsFeedWindowItem::onAppSkinChanged);

    escapeButton_ = new CommonGraphics::EscapeButton(this);
    connect(escapeButton_, &CommonGraphics::EscapeButton::clicked, this, &NewsFeedWindowItem::onBackArrowButtonClicked);

    backArrowButton_ = new IconButton(32, 32, "BACK_ARROW", "", this);
    connect(backArrowButton_, &IconButton::clicked, this, &NewsFeedWindowItem::onBackArrowButtonClicked);

    contentItem_ = new NewsContentItem(this, WINDOW_WIDTH - 32);

    scrollAreaItem_ = new CommonGraphics::ScrollArea(this, curHeight_ - 102, WINDOW_WIDTH);
    scrollAreaItem_->setItem(contentItem_);
    connect(contentItem_, &NewsContentItem::messageRead, this, &NewsFeedWindowItem::messageRead);

    bottomResizeItem_ = new CommonGraphics::ResizeBar(this);
    connect(bottomResizeItem_, &CommonGraphics::ResizeBar::resizeStarted, this, &NewsFeedWindowItem::onResizeStarted);
    connect(bottomResizeItem_, &CommonGraphics::ResizeBar::resizeChange, this, &NewsFeedWindowItem::onResizeChange);
    connect(bottomResizeItem_, &CommonGraphics::ResizeBar::resizeFinished, this, &NewsFeedWindowItem::resizeFinished);

    updatePositions();
    // trigger app skin change in case we start in van gogh mode
    onAppSkinChanged(preferences_->appSkin());
}

QRectF NewsFeedWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, curHeight_);
}

void NewsFeedWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // resize area background
    qreal initialOpacity = painter->opacity();
    painter->fillRect(boundingRect().adjusted(0, 286*G_SCALE, 0, -7*G_SCALE), QBrush(QColor(2, 13, 28)));

    QRect rcCaption;
    // base background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
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
        rcCaption = QRect(64 * G_SCALE, 2*G_SCALE, 200 * G_SCALE, 56 * G_SCALE);
    }
    else
    {
        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
        QSharedPointer<IndependentPixmap> pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
        pixmapHeader->draw(0, 27 * G_SCALE, painter);
        rcCaption = QRect(64 * G_SCALE, 30 * G_SCALE, 200 * G_SCALE, 56 * G_SCALE);
    }

    // draw page caption
    painter->setPen(Qt::white);
    QFont *font = FontManager::instance().getFont(16, true);
    painter->setFont(*font);
    painter->drawText(rcCaption, Qt::AlignLeft | Qt::AlignVCenter, tr("News Feed"));

    // bottom-most background
    painter->setOpacity(initialOpacity);
    if (roundedFooter_)
    {
        painter->setPen(footerColor_);
        painter->setBrush(footerColor_);
        painter->drawRoundedRect(getBottomResizeArea(), 8 * G_SCALE, 8 * G_SCALE);
        painter->fillRect(getBottomResizeArea().adjusted(0, -2 * G_SCALE, 0, -7 * G_SCALE), QBrush(footerColor_));
    }
    else
    {
        painter->fillRect(getBottomResizeArea(), QBrush(footerColor_));
    }
}

void NewsFeedWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    curHeight_ = curHeight_ * (G_SCALE / curScale_);
    curScale_ = G_SCALE;
    updatePositions();
}

void NewsFeedWindowItem::setClickable(bool isClickable)
{
    Q_UNUSED(isClickable);
}

void NewsFeedWindowItem::setMessagesWithCurrentOverride(const QVector<types::Notification> &arr,
                                                        const QSet<qint64> &shownIds,
                                                        int overrideCurrentMessageId)
{
    contentItem_->setMessagesWithCurrentOverride(arr, shownIds, overrideCurrentMessageId);
}

void NewsFeedWindowItem::setMessages(const QVector<types::Notification> &arr,
                                     const QSet<qint64> &shownIds)
{
    contentItem_->setMessages(arr, shownIds);
}

void NewsFeedWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit escClick();
    } else {
        scrollAreaItem_->handleKeyPressEvent(event);
    }
}

void NewsFeedWindowItem::onBackArrowButtonClicked()
{
    emit escClick();
}

void NewsFeedWindowItem::updatePositions()
{
    bottomResizeItem_->setPos(BOTTOM_RESIZE_ORIGIN_X*G_SCALE, curHeight_ - BOTTOM_RESIZE_OFFSET_Y*G_SCALE);
    escapeButton_->setPos(WINDOW_WIDTH*G_SCALE - escapeButton_->boundingRect().width() - 16*G_SCALE, 16*G_SCALE);

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        backArrowButton_->setPos(16*G_SCALE, 12*G_SCALE);
        scrollAreaItem_->setPos(0, 55*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 74*G_SCALE);
    }
    else
    {
        backArrowButton_->setPos(16*G_SCALE, 40 * G_SCALE);
        scrollAreaItem_->setPos(0, 83*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 102*G_SCALE);
    }
}

QGraphicsObject *NewsFeedWindowItem::getGraphicsObject()
{
    return this;
}

int NewsFeedWindowItem::recommendedHeight()
{
    return MIN_HEIGHT;
}

void NewsFeedWindowItem::setHeight(int height)
{
    prepareGeometryChange();
    curHeight_ = height;
    updateChildItemsAfterHeightChanged();
}

void NewsFeedWindowItem::setScrollBarVisibility(bool on)
{
    scrollAreaItem_->setScrollBarVisibility(on);
}

void NewsFeedWindowItem::onResizeStarted()
{
    heightAtResizeStart_ = curHeight_;
}

void NewsFeedWindowItem::onResizeChange(int y)
{
    if ((heightAtResizeStart_ + y) >= MIN_HEIGHT * G_SCALE)
    {
        prepareGeometryChange();
        curHeight_ = heightAtResizeStart_ + y;
        updateChildItemsAfterHeightChanged();

        emit sizeChanged();
    }
}

void NewsFeedWindowItem::updateChildItemsAfterHeightChanged()
{
    bottomResizeItem_->setPos(BOTTOM_RESIZE_ORIGIN_X*G_SCALE, curHeight_ - BOTTOM_RESIZE_OFFSET_Y*G_SCALE);
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        scrollAreaItem_->setHeight(curHeight_ - 74 * G_SCALE);
    }
    else
    {
        scrollAreaItem_->setHeight(curHeight_ - 102 * G_SCALE);
    }
}

QRectF NewsFeedWindowItem::getBottomResizeArea()
{
    return QRectF(0, curHeight_ - BOTTOM_AREA_HEIGHT * G_SCALE, boundingRect().width(), BOTTOM_AREA_HEIGHT * G_SCALE);
}

void NewsFeedWindowItem::updateRead()
{
    contentItem_->updateRead();
}

void NewsFeedWindowItem::onAppSkinChanged(APP_SKIN s)
{
    if (s == APP_SKIN_ALPHA)
    {
        escapeButton_->setTextPosition(CommonGraphics::EscapeButton::TEXT_POSITION_BOTTOM);
    }
    else if (s == APP_SKIN_VAN_GOGH)
    {
        escapeButton_->setTextPosition(CommonGraphics::EscapeButton::TEXT_POSITION_LEFT);
    }
    updatePositions();
    update();
}

} // namespace NewsFeedWindow
