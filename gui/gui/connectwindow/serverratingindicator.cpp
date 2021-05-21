#include "serverratingindicator.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"

namespace ConnectWindow {

ServerRatingIndicator::ServerRatingIndicator(ScalableGraphicsObject *parent)
    : ClickableGraphicsObject(parent), width_(0), height_(0),
      connectState_(ProtoTypes::DISCONNECTED), pingTime_(PingTime::PING_FAILED)
{
    setClickable(true);
}

QRectF ServerRatingIndicator::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void ServerRatingIndicator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 255)));
    if (connectState_ == ProtoTypes::CONNECTED)
    {
        painter->setOpacity(1.0);
        if (pingTime_.toConnectionSpeed() == 0)
        {
            painter->setOpacity(0.5 * initOpacity);
            pingOnFull_->draw(painter, 0, 0);
        }
        else if (pingTime_.toConnectionSpeed() == 1)
        {
            painter->setOpacity(1.0 * initOpacity);
            pingOnLow_->draw(painter, 0, 0);
        }
        else if (pingTime_.toConnectionSpeed() == 2)
        {
            painter->setOpacity(1.0 * initOpacity);
            pingOnHalf_->draw(painter, 0, 0);
        }
        else if (pingTime_.toConnectionSpeed() == 3)
        {
            painter->setOpacity(1.0 * initOpacity);
            pingOnFull_->draw(painter, 0, 0);
        }
    }
    else
    {
        if (pingTime_.toConnectionSpeed() == 0)
        {
            painter->setOpacity(0.5 * initOpacity);
            pingOffFull_->draw(painter, 0, 0);
        }
        else if (pingTime_.toConnectionSpeed() == 1)
        {
            painter->setOpacity(1.0 * initOpacity);
            pingOffLow_->draw(painter, 0, 0);
        }
        else if (pingTime_.toConnectionSpeed() == 2)
        {
            painter->setOpacity(1.0 * initOpacity);
            pingOffHalf_->draw(painter, 0, 0);
        }
        else if (pingTime_.toConnectionSpeed() == 3)
        {
            painter->setOpacity(1.0 * initOpacity);
            pingOffFull_->draw(painter, 0, 0);
        }
    }
}

void ServerRatingIndicator::onConnectStateChanged(ProtoTypes::ConnectStateType newConnectState, ProtoTypes::ConnectStateType prevConnectState)
{
    Q_UNUSED(prevConnectState);

    connectState_ = newConnectState;
    update();
}

void ServerRatingIndicator::setPingTime(const PingTime &pingTime)
{
    pingTime_ = pingTime;
    update();
}

void ServerRatingIndicator::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    pingOnFull_.reset(new ImageWithShadow("pingbar/ON_FULL","pingbar/SHADOW"));
    pingOnLow_.reset(new ImageWithShadow("pingbar/ON_LOW","pingbar/SHADOW"));
    pingOnHalf_.reset(new ImageWithShadow("pingbar/ON_HALF","pingbar/SHADOW"));
    pingOffFull_.reset(new ImageWithShadow("pingbar/OFF_FULL","pingbar/SHADOW"));
    pingOffLow_.reset(new ImageWithShadow("pingbar/OFF_LOW","pingbar/SHADOW"));
    pingOffHalf_.reset(new ImageWithShadow("pingbar/OFF_HALF","pingbar/SHADOW"));

    updateDimensions();
}


void ServerRatingIndicator::updateDimensions()
{
    width_ = pingOnFull_->width();
    height_ = pingOnFull_->height();
}

} //namespace ConnectWindow
