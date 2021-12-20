#include "generalmessagetwobuttonwindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"


namespace GeneralMessage {

GeneralMessageTwoButtonWindowItem::GeneralMessageTwoButtonWindowItem(const QString title,
                                                                     const QString icon,
                                                                     const QString acceptText,
                                                                     const QString rejectText,
                                                                     QGraphicsObject *parent) : ScalableGraphicsObject (parent)
  , title_(title)
  , icon_(icon)
  , titlePosY_(RECT_TITLE_POS_Y)
  , iconPosY_(RECT_ICON_POS_Y)
  , shapedToConnectWindow_(false)
  , isShutdownAnimationMode_(false)
  , selection_(NONE)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    acceptButton_ = new CommonGraphics::BubbleButtonDark(this, 108, 40, 20, 20);
    acceptButton_->setText(acceptText);
    acceptButton_->setFont(FontDescr(16, false));
    connect(acceptButton_, SIGNAL(clicked()), SIGNAL(acceptClick()));
    connect(acceptButton_, SIGNAL(hoverEnter()), SLOT(onHoverAccept()));
    connect(acceptButton_, SIGNAL(hoverLeave()), SLOT(onHoverLeaveAccept()));

    rejectButton_ = new CommonGraphics::BubbleButtonDark(this, 108, 40, 20, 20);
    rejectButton_->setText(rejectText);
    rejectButton_->setFont(FontDescr(16, false));
    connect(rejectButton_, SIGNAL(clicked()), SIGNAL(rejectClick()));
    connect(rejectButton_, SIGNAL(hoverEnter()), SLOT(onHoverReject()));
    connect(rejectButton_, SIGNAL(hoverLeave()), SLOT(onHoverLeaveReject()));

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    backgroundIcon_ = "background/WIN_MAIN_BG";
#else
    backgroundIcon_ = "background/MAC_MAIN_BG";
#endif

    titleShuttingDown_ = tr("Windscribe is shutting down");

    connect(&spinnerRotationAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSpinnerRotationChanged(QVariant)));
    connect(&spinnerRotationAnimation_, SIGNAL(finished()), SLOT(onSpinnerRotationFinished()));

    updateScaling();
}

QRectF GeneralMessageTwoButtonWindowItem::boundingRect() const
{
    return QRect(0,0,WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void GeneralMessageTwoButtonWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // qreal initialOpacity = painter->opacity();

    // background
    if (shapedToConnectWindow_)
    {
        QSharedPointer<IndependentPixmap> bkgdPix = ImageResourcesSvg::instance().getIndependentPixmap(backgroundIcon_);
        bkgdPix->draw(0, 0, painter);
    }
    else
    {
        QColor black = FontManager::instance().getMidnightColor();

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
        painter->fillRect(boundingRect(), black);
#else
        painter->setPen(black);
        painter->setBrush(black);
        painter->drawRoundedRect(boundingRect().adjusted(0,0,0,0), 5*G_SCALE, 5*G_SCALE);
#endif
    }

    if (!isShutdownAnimationMode_)
    {
        // icon
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(icon_);
        p->draw((WINDOW_WIDTH/2 - 20) * G_SCALE, iconPosY_*G_SCALE, painter);

        // title
        painter->setPen(Qt::white);
        QFont titleFont = *FontManager::instance().getFont(16, true);
        painter->setFont(titleFont);
        QRectF titleRect(0, titlePosY_ * G_SCALE, WINDOW_WIDTH*G_SCALE, CommonGraphics::textHeight(titleFont));
        painter->drawText(titleRect, Qt::AlignCenter, tr(title_.toStdString().c_str()));
    }
    else
    {
        // title
        painter->setPen(Qt::white);
        QFont titleFont = *FontManager::instance().getFont(16, true);
        painter->setFont(titleFont);
        QRectF titleRect(0, (titlePosY_ + 70) * G_SCALE, WINDOW_WIDTH*G_SCALE, CommonGraphics::textHeight(titleFont));
        painter->drawText(titleRect, Qt::AlignCenter, tr(titleShuttingDown_.toStdString().c_str()));

        curLogoPosY_ = LOGO_POS_CENTER;

        const int logoPosX = WINDOW_WIDTH/2 - 20;
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("login/BADGE_ICON");
        p->draw(logoPosX * G_SCALE, curLogoPosY_ * G_SCALE, painter);

        // spinner
        painter->setPen(QPen(Qt::white, 2 * G_SCALE));
        painter->translate((logoPosX + 20) * G_SCALE, (LOGO_POS_CENTER + 20) * G_SCALE);
        painter->rotate(curSpinnerRotation_);
        const int circleDiameter = 80 * G_SCALE;
        painter->drawArc(QRect(-circleDiameter/2, -circleDiameter/2, circleDiameter, circleDiameter), 0, 4 * 360);
    }
}

void GeneralMessageTwoButtonWindowItem::setTitle(const QString title)
{
    title_ = title;
    update();
}

void GeneralMessageTwoButtonWindowItem::setIcon(const QString icon)
{
    icon_ = icon;
    update();
}

void GeneralMessageTwoButtonWindowItem::setAcceptText(const QString text)
{
    acceptButton_->setText(text);
}

void GeneralMessageTwoButtonWindowItem::setRejectText(const QString text)
{
    rejectButton_->setText(text);
}

void GeneralMessageTwoButtonWindowItem::setBackgroundShapedToConnectWindow(bool shapedToConnectWindow)
{
    shapedToConnectWindow_ = shapedToConnectWindow;

    int titlePos = RECT_TITLE_POS_Y;
    int iconPos = RECT_ICON_POS_Y;
    if (shapedToConnectWindow)
    {
        titlePos = SHAPED_TITLE_POS_Y;
        iconPos =  SHAPED_ICON_POS_Y;
    }

    titlePosY_ = titlePos;
    iconPosY_ = iconPos;
    updateScaling();
}

void GeneralMessageTwoButtonWindowItem::setShutdownAnimationMode(bool isShutdownAnimationMode)
{
    isShutdownAnimationMode_ = isShutdownAnimationMode;

    acceptButton_->setVisible(!isShutdownAnimationMode_);
    rejectButton_->setVisible(!isShutdownAnimationMode_);

    if (isShutdownAnimationMode_)
    {
        curSpinnerRotation_ = 0;
        startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, (double)curSpinnerRotation_ / 360.0 * (double)SPINNER_SPEED);
    }

    update();
}

void GeneralMessageTwoButtonWindowItem::clearSelection()
{
    changeSelection(NONE);
}

void GeneralMessageTwoButtonWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    acceptButton_->setPos(WINDOW_WIDTH/2*G_SCALE - acceptButton_->boundingRect().width()/2, RECT_ACCEPT_BUTTON_POS_Y*G_SCALE);
    rejectButton_->setPos(WINDOW_WIDTH/2*G_SCALE - rejectButton_->boundingRect().width()/2, RECT_REJECT_BUTTON_POS_Y*G_SCALE);
}

void GeneralMessageTwoButtonWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (selection_ == ACCEPT)
        {
            emit acceptClick();
        }
        else if (selection_ == REJECT)
        {
            emit rejectClick();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        emit rejectClick();
    }
    else if (event->key() == Qt::Key_Up)
    {
        if (selection_ == REJECT)
        {
            changeSelection(ACCEPT);
        }
    }
    else if (event->key() == Qt::Key_Down)
    {
        if (selection_ == NONE)
        {
            changeSelection(ACCEPT);
        }
        else if (selection_ == ACCEPT)
        {
            changeSelection(REJECT);
        }
    }

    event->ignore();
}

bool GeneralMessageTwoButtonWindowItem::sceneEvent(QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Tab)
        {
            if (selection_ == NONE)
            {
                changeSelection(ACCEPT);
            }
            else if (selection_ == ACCEPT)
            {
                changeSelection(REJECT);
            }
            else if (selection_ == REJECT)
            {
                changeSelection(ACCEPT);
            }
            return true;
        }
    }

    QGraphicsItem::sceneEvent(event);
    return false;
}

void GeneralMessageTwoButtonWindowItem::onHoverAccept()
{
    unselectCurrentButton(rejectButton_);
    selection_ = ACCEPT;
}

void GeneralMessageTwoButtonWindowItem::onHoverLeaveAccept()
{
    unselectCurrentButton(acceptButton_);
    selection_ = NONE;
}

void GeneralMessageTwoButtonWindowItem::onHoverReject()
{
    unselectCurrentButton(acceptButton_);
    selection_ = REJECT;
}

void GeneralMessageTwoButtonWindowItem::onHoverLeaveReject()
{
    unselectCurrentButton(rejectButton_);
    selection_ = NONE;
}

void GeneralMessageTwoButtonWindowItem::onSpinnerRotationChanged(const QVariant &value)
{
    curSpinnerRotation_ = value.toInt();
    update();
}

void GeneralMessageTwoButtonWindowItem::onSpinnerRotationFinished()
{
    curSpinnerRotation_ = 0;
    startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, SPINNER_SPEED);
}

void GeneralMessageTwoButtonWindowItem::unselectCurrentButton(CommonGraphics::BubbleButtonDark *button)
{
    if (button != nullptr)
    {
        button->animateColorChange(Qt::white, FontManager::instance().getMidnightColor(), ANIMATION_SPEED_FAST);
        button->animateOpacityChange(OPACITY_UNHOVER_TEXT, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
}

void GeneralMessageTwoButtonWindowItem::changeSelection(Selection selection)
{
    if (selection != selection_)
    {
        unselectCurrentButton(acceptButton_);
        unselectCurrentButton(rejectButton_);

        // animate button selection
        if (selection == ACCEPT)
        {
            acceptButton_->animateColorChange(FontManager::instance().getMidnightColor(), Qt::white,ANIMATION_SPEED_FAST);
            acceptButton_->animateOpacityChange(OPACITY_FULL, OPACITY_FULL, ANIMATION_SPEED_FAST);
        }
        else if (selection == REJECT)
        {
            rejectButton_->animateColorChange(FontManager::instance().getMidnightColor(), Qt::white, ANIMATION_SPEED_FAST);
            rejectButton_->animateOpacityChange(OPACITY_FULL, OPACITY_FULL, ANIMATION_SPEED_FAST);
        }

        selection_ = selection;
    }
}

} // namespace GeneralMessage
