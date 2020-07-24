#include "connectbutton.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {


ConnectButton::ConnectButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent)
    , connectStateType_(ProtoTypes::DISCONNECTED)
    , online_(false)
    , splitRouting_(false)
{
    svgItemButton_ = new ImageItem("ON_BUTTON", this);
    svgItemButton_->setRotation(-180);

    svgItemConnectedRing_ = new ImageItem("RING_CONNECTED", this);
    svgItemConnectedRing_->setOpacity(0.0);

    svgItemConnectedSplitRoutingRing_ = new ImageItem("RING_CONNECTED_SPLIT_ROUTING", this);
    svgItemConnectedSplitRoutingRing_->setOpacity(0.0);

    svgItemConnectingRing_ = new ImageItem("RING_CONNECTING", this);
    svgItemConnectingRing_->setOpacity(0.0);

    svgItemConnectingNoInternetRing_ = new ImageItem("RING_NO_INTERNET", this);
    svgItemConnectingNoInternetRing_->setOpacity(0.0);

    buttonRotationAnimation_.setTargetObject(svgItemButton_);
    buttonRotationAnimation_.setPropertyName("rotation");
    buttonRotationAnimation_.setStartValue(-180);
    buttonRotationAnimation_.setEndValue(0);
    buttonRotationAnimation_.setDuration(150);

    connectingRingRotationAnimation_.setTargetObject(svgItemConnectingRing_);
    connectingRingRotationAnimation_.setPropertyName("rotation");
    connectingRingRotationAnimation_.setStartValue(0);
    connectingRingRotationAnimation_.setEndValue(360);
    connectingRingRotationAnimation_.setDuration(1200);
    connectingRingRotationAnimation_.setLoopCount(-1);

    connectingRingOpacityAnimation_.setTargetObject(svgItemConnectingRing_);
    connectingRingOpacityAnimation_.setPropertyName("opacity");
    connectingRingOpacityAnimation_.setStartValue(0.0);
    connectingRingOpacityAnimation_.setEndValue(1.0);
    connectingRingOpacityAnimation_.setDuration(200);
    connect(&connectingRingOpacityAnimation_, SIGNAL(finished()), SLOT(onConnectingRingOpacityAnimationFinished()));

    noInternetRingRotationAnimation_.setTargetObject(svgItemConnectingNoInternetRing_);
    noInternetRingRotationAnimation_.setPropertyName("rotation");
    noInternetRingRotationAnimation_.setStartValue(0);
    noInternetRingRotationAnimation_.setEndValue(360);
    noInternetRingRotationAnimation_.setDuration(1200);
    noInternetRingRotationAnimation_.setLoopCount(-1);

    noInternetRingOpacityAnimation_.setTargetObject(svgItemConnectingNoInternetRing_);
    noInternetRingOpacityAnimation_.setPropertyName("opacity");
    noInternetRingOpacityAnimation_.setStartValue(0.0);
    noInternetRingOpacityAnimation_.setEndValue(1.0);
    noInternetRingOpacityAnimation_.setDuration(200);
    connect(&noInternetRingOpacityAnimation_, SIGNAL(finished()), SLOT(onNoInternetRingOpacityAnimationFinished()));

    connectedRingOpacityAnimation_.setTargetObject(svgItemConnectedRing_);
    connectedRingOpacityAnimation_.setPropertyName("opacity");
    connectedRingOpacityAnimation_.setStartValue(0.0);
    connectedRingOpacityAnimation_.setEndValue(1.0);
    connectedRingOpacityAnimation_.setDuration(100);

    connectedSplitRoutingRingOpacityAnimation_.setTargetObject(svgItemConnectedSplitRoutingRing_);
    connectedSplitRoutingRingOpacityAnimation_.setPropertyName("opacity");
    connectedSplitRoutingRingOpacityAnimation_.setStartValue(0.0);
    connectedSplitRoutingRingOpacityAnimation_.setEndValue(1.0);
    connectedSplitRoutingRingOpacityAnimation_.setDuration(100);

    setClickable(true);
    updatePositions();
}


QRectF ConnectButton::boundingRect() const
{
    return QRectF(0, 0, 82*G_SCALE, 82*G_SCALE);
}

void ConnectButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}


void ConnectButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    updatePositions();
}

void ConnectButton::onConnectStateChanged(ProtoTypes::ConnectStateType newConnectState, ProtoTypes::ConnectStateType prevConnectState)
{
    Q_UNUSED(prevConnectState);


    // flip the on button
    if (newConnectState == ProtoTypes::CONNECTING || newConnectState == ProtoTypes::CONNECTED)
    {
        buttonRotationAnimation_.setDirection(QPropertyAnimation::Forward);
        if (buttonRotationAnimation_.state() != QPropertyAnimation::Running)
        {
            if (!qFuzzyIsNull(svgItemButton_->rotation()))
            {
                buttonRotationAnimation_.start();
            }
        }
    }
    else
    {
        buttonRotationAnimation_.setDirection(QPropertyAnimation::Backward);
        if (buttonRotationAnimation_.state() != QPropertyAnimation::Running)
        {
            if (!qFuzzyCompare(svgItemButton_->rotation(), -180))
            {
                buttonRotationAnimation_.start();
            }
        }
    }

    if (newConnectState == ProtoTypes::CONNECTING)
    {
        fuzzyHideConnectedRing(); // might go from connected to connecting

        if (online_)
        {
            if (newConnectState != connectStateType_) // prevent double sending of CONNECTING from causing flash
            {
                startConnectingRingAnimations(QPropertyAnimation::Forward);
            }
        }
        else
        {
            startNoInternetRingAnimations(QPropertyAnimation::Forward);
        }
    }
    else if (newConnectState == ProtoTypes::CONNECTED)
    {
        fuzzyHideConnectingRing();

        if (online_)
        {
            fuzzyHideNoInternetRing();
            startConnectedRingAnimations(QPropertyAnimation::Forward);
        }
    }
    else if (newConnectState == ProtoTypes::DISCONNECTED || newConnectState == ProtoTypes::DISCONNECTING)
    {
        fuzzyHideConnectedRing();
        fuzzyHideConnectingRing();
        fuzzyHideNoInternetRing();
    }

    connectStateType_ = newConnectState;
}

void ConnectButton::setInternetConnectivity(bool online)
{
    if (online != online_)
    {
        if (online)
        {
            if (connectStateType_ == ProtoTypes::CONNECTED)
            {
                noInternetRingOpacityAnimation_.stop();
                startNoInternetRingAnimations(QPropertyAnimation::Backward);
                startConnectedRingAnimations(QPropertyAnimation::Forward);
            }
            else if (connectStateType_ == ProtoTypes::CONNECTING)
            {
                noInternetRingOpacityAnimation_.stop();
                startNoInternetRingAnimations(QPropertyAnimation::Backward);
                startConnectingRingAnimations(QPropertyAnimation::Forward);
            }
        }
        else
        {
            if (connectStateType_ == ProtoTypes::CONNECTED)
            {
                fuzzyHideConnectedRing();
                startNoInternetRingAnimations(QPropertyAnimation::Forward);
            }
            else if (connectStateType_ == ProtoTypes::CONNECTING)
            {
                connectingRingOpacityAnimation_.stop();
                startConnectingRingAnimations(QPropertyAnimation::Backward);
                startNoInternetRingAnimations(QPropertyAnimation::Forward);
            }
        }
    }

    online_ = online;
}

void ConnectButton::setSplitRouting(bool on)
{
    splitRouting_ = on;

    if (connectStateType_ == ProtoTypes::CONNECTED)
    {
        fuzzyHideConnectedRing();
        startConnectedRingAnimations(QPropertyAnimation::Forward);
    }
}

void ConnectButton::onConnectingRingOpacityAnimationFinished()
{
    if (connectingRingOpacityAnimation_.direction() == QPropertyAnimation::Backward)
    {
        connectingRingRotationAnimation_.stop();
    }
}

void ConnectButton::updatePositions()
{

    double connectingWidth = svgItemConnectingRing_->boundingRect().width();
    double transformOrigin = connectingWidth / 2;
    double buttonTransform = svgItemButton_->boundingRect().width()/2;
    double buttonPos = (connectingWidth - svgItemButton_->boundingRect().width())/2;

    //qDebug() << "Connecting size: " << connectingWidth;
    //qDebug() << "Button transform: " << buttonTransform;
    //qDebug() << "Button pos: " << buttonPos;

    svgItemButton_->setPos(buttonPos, buttonPos);
    svgItemButton_->setTransformOriginPoint(buttonTransform, buttonTransform);

    svgItemConnectedRing_->setPos(0, 0);
    svgItemConnectedRing_->setTransformOriginPoint(transformOrigin, transformOrigin);

    svgItemConnectedSplitRoutingRing_->setPos(0,0);
    svgItemConnectedSplitRoutingRing_->setTransformOriginPoint(transformOrigin, transformOrigin);

    svgItemConnectingRing_->setPos(0, 0);
    svgItemConnectingRing_->setTransformOriginPoint(transformOrigin, transformOrigin);

    svgItemConnectingNoInternetRing_->setPos(0, 0);
    svgItemConnectingNoInternetRing_->setTransformOriginPoint(transformOrigin, transformOrigin);
}

void ConnectButton::onNoInternetRingOpacityAnimationFinished()
{
    if (noInternetRingOpacityAnimation_.direction() == QPropertyAnimation::Backward)
    {
        noInternetRingRotationAnimation_.stop();
    }
}

void ConnectButton::fuzzyHideConnectedRing()
{
    // connected plain
    connectedRingOpacityAnimation_.setDirection(QPropertyAnimation::Backward);
    if (connectedRingOpacityAnimation_.state() != QPropertyAnimation::Running)
    {
        if (!qFuzzyIsNull(svgItemConnectedRing_->opacity()))
        {
            connectedRingOpacityAnimation_.start();
        }
    }

    // connected split routing
    connectedSplitRoutingRingOpacityAnimation_.setDirection(QPropertyAnimation::Backward);
    if (connectedSplitRoutingRingOpacityAnimation_.state() != QPropertyAnimation::Running)
    {
        if (!qFuzzyIsNull(svgItemConnectedSplitRoutingRing_->opacity()))
        {
            connectedSplitRoutingRingOpacityAnimation_.start();
        }
    }
}

void ConnectButton::fuzzyHideConnectingRing()
{
    connectingRingOpacityAnimation_.setDirection(QPropertyAnimation::Backward);
    if (connectingRingOpacityAnimation_.state() != QPropertyAnimation::Running)
    {
        if (!qFuzzyIsNull(svgItemConnectingRing_->opacity()))
        {
            connectingRingOpacityAnimation_.start();
        }
    }
}

void ConnectButton::fuzzyHideNoInternetRing()
{
    noInternetRingOpacityAnimation_.setDirection(QPropertyAnimation::Backward);
    if (noInternetRingOpacityAnimation_.state() != QPropertyAnimation::Running)
    {
        if (!qFuzzyIsNull(svgItemConnectingNoInternetRing_->opacity()))
        {
            noInternetRingOpacityAnimation_.start();
        }
    }
}

void ConnectButton::startConnectedRingAnimations(QAbstractAnimation::Direction dir)
{
    if (splitRouting_)
    {
        connectedSplitRoutingRingOpacityAnimation_.setDirection(dir);
        if (connectedSplitRoutingRingOpacityAnimation_.state() != QPropertyAnimation::Running)
        {
            connectedSplitRoutingRingOpacityAnimation_.start();
        }
    }
    else
    {
        connectedRingOpacityAnimation_.setDirection(dir);
        if (connectedRingOpacityAnimation_.state() != QPropertyAnimation::Running)
        {
            connectedRingOpacityAnimation_.start();
        }
    }
}

void ConnectButton::startConnectingRingAnimations(QPropertyAnimation::Direction dir)
{
    connectingRingRotationAnimation_.start();
    connectingRingOpacityAnimation_.setDirection(dir);
    if (connectingRingOpacityAnimation_.state() != QPropertyAnimation::Running)
    {
        connectingRingOpacityAnimation_.start();
    }
}

void ConnectButton::startNoInternetRingAnimations(QPropertyAnimation::Direction dir)
{
    noInternetRingRotationAnimation_.start();
    noInternetRingOpacityAnimation_.setDirection(dir);
    if (noInternetRingOpacityAnimation_.state() != QPropertyAnimation::Running)
    {
        noInternetRingOpacityAnimation_.start();
    }
}

} //namespace ConnectWindow
