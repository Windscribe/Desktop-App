#include "newsfeedwindowitem.h"

#include <QPainter>
#include <QTimer>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace NewsFeedWindow {

const int MESSAGE_POS_X = 12;
const int MESSAGE_POS_Y = 125;

NewsFeedWindowItem::NewsFeedWindowItem(QGraphicsObject *parent,
                                       PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), messageIdIsInitialized_(false)
{
    Q_ASSERT(preferencesHelper);
    setFlags(QGraphicsObject::ItemIsFocusable);

    setVisible(false);

    windowTitle_ = QT_TR_NOOP("News Feed");

    int scrollWidth = WINDOW_WIDTH - 15;
    messageItem_ = new ScrollableMessage(scrollWidth, 115, this);

#ifdef Q_OS_WIN
    mainBackground_   = "background/WIN_MAIN_BG";
    locationButtonBG_ = "background/WIN_LOCATION_BUTTON_BG_BRIGHT";
    headerBackground_ = "background/WIN_HEADER_BG_OVERLAY";
#else
    mainBackground_   = "background/MAC_MAIN_BG";
    locationButtonBG_ = "background/MAC_LOCATION_BUTTON_BG_BRIGHT";
    headerBackground_ = "background/MAC_HEADER_BG_OVERLAY";
#endif

    curBackgroundOpacity_ = OPACITY_FULL;
    curHeaderBackgroundOpacity_ = OPACITY_NEWSFEED_HEADER_BACKGROUND;
    curMessageTitleOpacity_ = OPACITY_UNHOVER_TEXT;

    curMessageOpacity_ = OPACITY_FULL;
    curDefaultOpacity_ = OPACITY_FULL;

    // upper buttons
    backArrowButton_ = new IconButton(20, 24, "login/BACK_ARROW", "", this);
    connect(backArrowButton_, SIGNAL(clicked()), SLOT(onBackArrowButtonClicked()));

#ifdef Q_OS_WIN
    closeButton_ = new IconButton(10, 10, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, SIGNAL(clicked()), SIGNAL(closeClick()));

    minimizeButton_ = new IconButton(10, 10, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, SIGNAL(clicked()), SIGNAL(minimizeClick()));
#elif defined Q_OS_MAC

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", this);
    connect(minimizeButton_, SIGNAL(clicked()), SIGNAL(minimizeClick()));
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(false);

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", this);
    connect(closeButton_, SIGNAL(clicked()), SIGNAL(closeClick()));
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(false);
#endif
    // bottom right buttons
    rightArrowButton_ = new IconButton(16, 16, "newsfeed/RIGHT_ARROW", "", this);
    rightArrowButton_->animateOpacityChange(OPACITY_UNHOVER_ICON_STANDALONE, 50);
    connect(rightArrowButton_, SIGNAL(clicked()), SLOT(onRightClick()));

    leftArrowButton_ = new IconButton(16, 16, "newsfeed/LEFT_ARROW", "", this);
    leftArrowButton_->animateOpacityChange(OPACITY_UNHOVER_ICON_STANDALONE, 50);
    connect(leftArrowButton_, SIGNAL(clicked()), SLOT(onLeftClick()));

    connect(preferencesHelper, SIGNAL(isDockedModeChanged(bool)), this,
            SLOT(onDockedModeChanged(bool)));

    installEventFilter(this);
    updatePositions();
}

QRectF NewsFeedWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, WINDOW_HEIGHT*G_SCALE);
}

void NewsFeedWindowItem::setClickable(bool isClickable)
{
    minimizeButton_->setClickable(isClickable);
    closeButton_->setClickable(isClickable);
    backArrowButton_->setClickable(isClickable);
    rightArrowButton_->setClickable(isClickable);
    leftArrowButton_->setClickable(isClickable);
    messageItem_->setEnabled(isClickable);
}

void NewsFeedWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    if (!messageIdIsInitialized_)
    {
        return;
    }
    qreal initialOpacity = painter->opacity();

    // main background
    painter->setOpacity(curBackgroundOpacity_ * initialOpacity);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(mainBackground_);
    p->draw(0, 0, painter);

    // header background
    painter->setOpacity(initialOpacity);
    QSharedPointer<IndependentPixmap> p3 = ImageResourcesSvg::instance().getIndependentPixmap(headerBackground_);
    p3->draw(0, 27*G_SCALE, painter);

    QSharedPointer<IndependentPixmap> p4 = ImageResourcesSvg::instance().getIndependentPixmap(locationButtonBG_);
    p4->draw(93*G_SCALE, 242*G_SCALE, painter);

    // Message header
    painter->setOpacity(curMessageTitleOpacity_ * initialOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, true));
    painter->drawText(MARGIN*G_SCALE, 115*G_SCALE, tr(messageTitle_.toStdString().c_str()));

    // News Feed Header
    painter->setOpacity(curDefaultOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(16, true));
    painter->drawText(56*G_SCALE, 62*G_SCALE, tr(windowTitle_.toStdString().c_str()));

    // Num Pages Text
    painter->setFont(*FontManager::instance().getFont(14, true));
    QString numberOfTotalPages = QString("%1 of %2").arg(messages_.api_notifications_size() - curMessageInd_).arg(messages_.api_notifications_size());
    painter->drawText(130*G_SCALE, 272*G_SCALE, numberOfTotalPages);

    // Vertical line
    painter->setOpacity(OPACITY_UNHOVER_DIVIDER * curDefaultOpacity_ * initialOpacity);
    painter->drawLine((WINDOW_WIDTH - 50 - 1)*G_SCALE, 269*G_SCALE, (WINDOW_WIDTH - 50 - 1)*G_SCALE, 290*G_SCALE);
    painter->drawLine((WINDOW_WIDTH - 50)*G_SCALE,     269*G_SCALE, (WINDOW_WIDTH - 50    )*G_SCALE, 290*G_SCALE);
}

void NewsFeedWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    updatePositions();
}

void NewsFeedWindowItem::setMessagesWithCurrentOverride(const ProtoTypes::ArrayApiNotification &arr,
                                                        const QSet<qint64> &shownIds,
                                                        int overrideCurrentMessageId)
{
    curMessageId_ = overrideCurrentMessageId;
    messageIdIsInitialized_ = true;
    setMessages(arr, shownIds);
    if (isVisible())
    {
        setCurrentMessageRead();
    }
}

void NewsFeedWindowItem::setMessages(const ProtoTypes::ArrayApiNotification &arr,
                                     const QSet<qint64> &shownIds)
{
    Q_ASSERT(arr.api_notifications_size() > 0);

    messages_ = arr;
    shownIds_ = shownIds;

    bool bFinded = false;
    if (messageIdIsInitialized_)
    {
        for (int i = 0; i < arr.api_notifications_size(); ++i)
        {
            if (arr.api_notifications(i).id() == curMessageId_)
            {
                curMessageInd_ = i;
                bFinded = true;
                break;
            }
        }
    }

    if (!bFinded)
    {
        curMessageInd_ = 0;
        curMessageId_ = messages_.api_notifications(0).id();
        messageIdIsInitialized_ = true;
        if (isVisible())
        {
            setCurrentMessageRead();
        }
    }

    setCurrentMessageToFirstUnread();
    updateCurrentMessage();
}

bool NewsFeedWindowItem::eventFilter(QObject *watching, QEvent *event)
{
    Q_UNUSED(watching);

    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Right) {
            onRightClick();
            return true;
        } else if (keyEvent->key() == Qt::Key_Left) {
            onLeftClick();
            return true;
        }
    } else if (event->type() == QEvent::FocusIn) {
        if (messageIdIsInitialized_)
            setCurrentMessageRead();
    }

    return false;
}

void NewsFeedWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit escClick();
    }
}

void NewsFeedWindowItem::onBackArrowButtonClicked()
{
    emit escClick();
}

void NewsFeedWindowItem::onLeftClick()
{
    if (curMessageInd_ < (messages_.api_notifications_size() - 1))
    {
        curMessageInd_++;
        curMessageId_ = messages_.api_notifications(curMessageInd_).id();
        updateCurrentMessage();
        setCurrentMessageRead();
    }
}

void NewsFeedWindowItem::onRightClick()
{
    if (curMessageInd_ > 0)
    {
        curMessageInd_--;
        curMessageId_ = messages_.api_notifications(curMessageInd_).id();
        updateCurrentMessage();
        setCurrentMessageRead();
    }
}

void NewsFeedWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MAC)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void NewsFeedWindowItem::setCurrentMessageToFirstUnread()
{
    qint64 curMessageDate = 0;
    for (int i = 0; i < messages_.api_notifications_size(); ++i) {
        const auto &notification = messages_.api_notifications(i);
        if (notification.date() >= curMessageDate && !shownIds_.contains(notification.id())) {
            curMessageInd_ = i;
            curMessageId_ = notification.id();
            curMessageDate = notification.date();
        }
    }
}

void NewsFeedWindowItem::setCurrentMessageRead()
{
    shownIds_.insert(curMessageId_);
    emit messageReaded(curMessageId_);
}

void NewsFeedWindowItem::updateCurrentMessage()
{
    messageTitle_ = QString::fromStdString(messages_.api_notifications(curMessageInd_).title());
    messageItem_->setMessage(QString::fromStdString(messages_.api_notifications(curMessageInd_).message()));
    update();
    updateLeftRightArrowClickability();
}


void NewsFeedWindowItem::updateLeftRightArrowClickability()
{
    if (curMessageInd_ == 0) // first
    {
        rightArrowButton_->setClickable(false);
        rightArrowButton_->animateOpacityChange(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
    }
    else
    {
        rightArrowButton_->setClickable(true);
    }

    if (curMessageInd_ == messages_.api_notifications_size() - 1) // last
    {
        leftArrowButton_->setClickable(false);
        leftArrowButton_->animateOpacityChange(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
    }
    else
    {
        leftArrowButton_->setClickable(true);
    }
}

void NewsFeedWindowItem::updatePositions()
{
    messageItem_->setPos(MESSAGE_POS_X*G_SCALE, MESSAGE_POS_Y*G_SCALE);

    backArrowButton_->setPos(16*G_SCALE, 45*G_SCALE);

#ifdef Q_OS_WIN
    int closePosX = (WINDOW_WIDTH - WINDOW_MARGIN)*G_SCALE - closeButton_->boundingRect().width();
    closeButton_->setPos(closePosX, 14*G_SCALE);
    minimizeButton_->setPos(closePosX - 26*G_SCALE - minimizeButton_->boundingRect().width(), 14*G_SCALE);
#elif defined Q_OS_MAC
    int rightMostPosX = static_cast<int>(boundingRect().width()) - static_cast<int>(closeButton_->boundingRect().width()) - 8*G_SCALE;
    int middlePosX = rightMostPosX - static_cast<int>(closeButton_->boundingRect().width()) - 8*G_SCALE;
    closeButton_->setPos(rightMostPosX, 8*G_SCALE);
    minimizeButton_->setPos(middlePosX, 8*G_SCALE);
#endif
    int rightArrowPosX = (WINDOW_WIDTH - 16 - WINDOW_MARGIN)*G_SCALE;
    rightArrowButton_->setPos(rightArrowPosX, 258*G_SCALE);
    leftArrowButton_->setPos(rightArrowPosX - (34 + 16)*G_SCALE, 258*G_SCALE);

    // updateCurrentMessage();

}

} // namespace NewsFeedWindow
