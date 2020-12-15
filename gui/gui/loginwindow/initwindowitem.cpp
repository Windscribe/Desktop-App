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
  , curAbortOpacity_(OPACITY_HIDDEN)
  , waitingAnimationActive_(false)
  , cropHeight_(0)
  , height_(WINDOW_HEIGHT)
  , logoDist_(0)
{
    connect(&spinnerRotationAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSpinnerRotationChanged(QVariant)));
    connect(&spinnerRotationAnimation_, SIGNAL(finished()), SLOT(onSpinnerRotationFinished()));
    connect(&logoPosAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onLogoPosChanged(QVariant)));
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
#ifdef Q_OS_WIN
    painter->fillRect(croppedRect, black);
#else
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(croppedRect.adjusted(0,0,0,0), 5*G_SCALE, 5*G_SCALE);
#endif

    // additional message
    if (!msg_.isEmpty())
    {
        painter->setFont(*FontManager::instance().getFont(16, false, 105));
        //painter->setPen(FontManager::instance().getMidnightColor());
        painter->setPen(Qt::white);
        int textPosY = (LOGO_POS_CENTER + 55)*G_SCALE;
        painter->drawText(QRect(0, textPosY , WINDOW_WIDTH*G_SCALE, height_*G_SCALE - textPosY), Qt::AlignCenter, msg_);
    }

    // logo
    //const int logoPosX = WINDOW_WIDTH/2*G_SCALE - 20 * G_SCALE;
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("login/BADGE_ICON");
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
    curLogoPosY_ = LOGO_POS_CENTER;
    waitingAnimationActive_ = false;

    //abortButton_->quickHide();
}

void InitWindowItem::startSlideAnimation()
{
    waitingAnimationActive_ = false;
    spinnerRotationAnimation_.stop();

    logoDist_ = curLogoPosY_ - LOGO_POS_TOP;
    startAnAnimation<int>(logoPosAnimation_, curLogoPosY_, LOGO_POS_TOP, ANIMATION_SPEED_FAST);
}

void InitWindowItem::startWaitingAnimation()
{
    waitingAnimationActive_ = true;

    curSpinnerRotation_ = 0;
    startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, (double)curSpinnerRotation_ / 360.0 * (double)SPINNER_SPEED);

    //showAbortTimer_.start();
}

void InitWindowItem::setClickable(bool /*clickable*/)
{
    //abortButton_->setClickable(clickable);
}

void InitWindowItem::setAdditionalMessage(const QString &msg)
{
    msg_ = msg;
    update();
}

void InitWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
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

void InitWindowItem::onShowAbortTimerTimeout()
{
    //abortButton_->animateShow(ANIMATION_SPEED_FAST);
}

}
