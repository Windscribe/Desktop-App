#include "serverratingindicator.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"

namespace ConnectWindow {

ServerRatingIndicator::ServerRatingIndicator(ScalableGraphicsObject *parent)
    : ClickableGraphicsObject(parent), width_(0), height_(0),
      connectState_(ProtoTypes::DISCONNECTED), pingTime_(PingTime::PING_FAILED)
{
    setClickable(true);
    updateDimensions();
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
    QSharedPointer<IndependentPixmap> pixmap;
    if (connectState_ == ProtoTypes::CONNECTED)
    {
        painter->setOpacity(1.0);
        if (pingTime_.toConnectionSpeed() == 0)
        {
            painter->setOpacity(0.5 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_ON_FULL");
        }
        else if (pingTime_.toConnectionSpeed() == 1)
        {
            painter->setOpacity(1.0 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_ON_LOW");
        }
        else if (pingTime_.toConnectionSpeed() == 2)
        {
            painter->setOpacity(1.0 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_ON_HALF");
        }
        else if (pingTime_.toConnectionSpeed() == 3)
        {
            painter->setOpacity(1.0 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_ON_FULL");
        }
    }
    else
    {
        if (pingTime_.toConnectionSpeed() == 0)
        {
            painter->setOpacity(0.5 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_OFF_FULL");
        }
        else if (pingTime_.toConnectionSpeed() == 1)
        {
            painter->setOpacity(1.0 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_OFF_LOW");
        }
        else if (pingTime_.toConnectionSpeed() == 2)
        {
            painter->setOpacity(1.0 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_OFF_HALF");
        }
        else if (pingTime_.toConnectionSpeed() == 3)
        {
            painter->setOpacity(1.0 * initOpacity);
            pixmap = ImageResourcesSvg::instance().getIndependentPixmap("PING_OFF_FULL");
        }
    }

    if (pixmap)
    {
        pixmap->draw(0, 0, painter);
    }
    else
    {
        Q_ASSERT(false);
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
    updateDimensions();
}


void ServerRatingIndicator::updateDimensions()
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("PING_ON_FULL");
    width_ = p->width();
    height_ = p->height();
}

} //namespace ConnectWindow
