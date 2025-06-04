#include "initwindowitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

InitWindowItem::InitWindowItem(QGraphicsObject *parent) : ScalableGraphicsObject (parent)
  , curLogoPosY_(LOGO_POS_CENTER)
  , curSpinnerRotation_(0)
  , curSpinnerOpacity_(1.0)
  , waitingAnimationActive_(false)
  , cropHeight_(0)
  , height_(WINDOW_HEIGHT)
  , logoDist_(0)
  , isgMsgSmallFont_(false)
{
#ifdef Q_OS_WIN
    closeButton_ = new IconButton(10, 10, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &InitWindowItem::abortClicked);
#else
    closeButton_ = new IconButton(14, 14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &InitWindowItem::abortClicked);
    connect(closeButton_, &IconButton::hoverEnter, [=]() { closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=]() { closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);
#endif
    connect(&spinnerRotationAnimation_, &QVariantAnimation::valueChanged, this, &InitWindowItem::onSpinnerRotationChanged);
    connect(&spinnerRotationAnimation_, &QVariantAnimation::finished, this, &InitWindowItem::onSpinnerRotationFinished);
    connect(&logoPosAnimation_, &QVariantAnimation::valueChanged, this, &InitWindowItem::onLogoPosChanged);

    updatePositions();
}

QRectF InitWindowItem::boundingRect() const
{
    return QRect(0, 0, WINDOW_WIDTH * G_SCALE, height_ * G_SCALE);
}

void InitWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF unscaledRect = QRectF(0, 0, WINDOW_WIDTH*G_SCALE, height_);
    QRectF croppedRect = unscaledRect;
    croppedRect.setY(cropHeight_*G_SCALE);
    croppedRect.setHeight((height_ - 2 * cropHeight_)*G_SCALE);

    // background
    QColor black = FontManager::instance().getMidnightColor();
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(croppedRect, 9*G_SCALE, 9*G_SCALE);

    // additional message
    if (!msg_.isEmpty())
    {
        const int kFontSize = isgMsgSmallFont_ ? MSG_SMALL_FONT : MSG_LARGE_FONT;
        painter->setFont(FontManager::instance().getFont(kFontSize, QFont::Normal, 105));
        //painter->setPen(FontManager::instance().getMidnightColor());
        painter->setPen(Qt::white);
        int textPosY = (LOGO_POS_CENTER + 55)*G_SCALE;
        painter->drawText(QRect(0, textPosY , WINDOW_WIDTH*G_SCALE, height_*G_SCALE - textPosY), Qt::AlignCenter, msg_);
    }

    // logo
    //const int logoPosX = WINDOW_WIDTH/2*G_SCALE - 20 * G_SCALE;
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("login/BADGE_ICON");
    int logoPosX = (boundingRect().width() - p->width()) / 2;
    p->draw(logoPosX, curLogoPosY_ * G_SCALE, painter);

    // spinner
    painter->setPen(QPen(Qt::white, 2 * G_SCALE));
    painter->translate(logoPosX + p->width() / 2, LOGO_POS_CENTER * G_SCALE + p->height() / 2);
    painter->rotate(curSpinnerRotation_);
    const int circleDiameter = 80 * G_SCALE;
    painter->setOpacity(curSpinnerOpacity_);
    painter->drawArc(QRect(-circleDiameter/2, -circleDiameter/2, circleDiameter, circleDiameter), 0, 4 * 360);
}

void InitWindowItem::resetState()
{
    closeButton_->setVisible(false);
    curLogoPosY_ = LOGO_POS_CENTER;
    waitingAnimationActive_ = false;

    //abortButton_->quickHide();
}

void InitWindowItem::startSlideAnimation()
{
    closeButton_->setVisible(false);
    waitingAnimationActive_ = false;
    spinnerRotationAnimation_.stop();

    logoDist_ = curLogoPosY_ - LOGO_POS_TOP;
    startAnAnimation<int>(logoPosAnimation_, curLogoPosY_, LOGO_POS_TOP, ANIMATION_SPEED_FAST);
}

void InitWindowItem::startWaitingAnimation()
{
    closeButton_->setVisible(false);
    waitingAnimationActive_ = true;

    curSpinnerRotation_ = 0;
    startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, (double)curSpinnerRotation_ / 360.0 * (double)SPINNER_SPEED);
}

void InitWindowItem::setAdditionalMessage(const QString &msg, bool useSmallFont)
{
    msg_ = msg;
    isgMsgSmallFont_ = useSmallFont;
    update();
}

void InitWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void InitWindowItem::setCropHeight(int height)
{
    cropHeight_ = height;
    update();
}

void InitWindowItem::setHeight(int height)
{
    prepareGeometryChange();
    height_ = height;
    update();
}

void InitWindowItem::setCloseButtonVisible(bool visible)
{
    closeButton_->setVisible(visible);
}

void InitWindowItem::onSpinnerRotationChanged(const QVariant &value)
{
    curSpinnerRotation_ = value.toInt();
    update();
}

void InitWindowItem::onSpinnerRotationFinished()
{
    if (waitingAnimationActive_)
    {
        curSpinnerRotation_ = 0;
        startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, SPINNER_SPEED);
    }
}

void InitWindowItem::onLogoPosChanged(const QVariant &value)
{
    curLogoPosY_ = value.toInt();
    if (logoDist_ > 0)
    {
        curSpinnerOpacity_ = (double)(curLogoPosY_ - LOGO_POS_TOP) / (double)logoDist_;
    }
    update();
}

void InitWindowItem::updatePositions()
{
#ifdef Q_OS_WIN
    const int kClosePosY = WINDOW_WIDTH * G_SCALE - closeButton_->boundingRect().width()
        - WINDOW_MARGIN * G_SCALE;
    closeButton_->setPos(kClosePosY, 14 * G_SCALE);
#else
    const int kClosePosY = static_cast<int>(boundingRect().width())
        - static_cast<int>(closeButton_->boundingRect().width()) - 8 * G_SCALE;
    closeButton_->setPos(kClosePosY, 8 * G_SCALE);
#endif
}

}
