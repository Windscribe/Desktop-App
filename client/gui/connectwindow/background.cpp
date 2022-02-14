#include "background.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QtGlobal>
#include <QElapsedTimer>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "utils/makecustomshadow.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {


Background::Background(ScalableGraphicsObject *parent, Preferences *preferences) : ScalableGraphicsObject(parent),
    opacityConnecting_(0), opacityConnected_(0), opacityDisconnected_(1), backgroundImage_(this, preferences)
{
    opacityConnectingAnimation_.setTargetObject(this);
    opacityConnectingAnimation_.setPropertyName("opacityConnecting");
    opacityConnectingAnimation_.setStartValue(0.0);
    opacityConnectingAnimation_.setEndValue(1.0);
    opacityConnectingAnimation_.setDuration(ANIMATION_DURATION);

    opacityConnectedAnimation_.setTargetObject(this);
    opacityConnectedAnimation_.setPropertyName("opacityConnected");
    opacityConnectedAnimation_.setStartValue(0.0);
    opacityConnectedAnimation_.setEndValue(1.0);
    opacityConnectedAnimation_.setDuration(ANIMATION_DURATION);

    opacityDisconnectedAnimation_.setTargetObject(this);
    opacityDisconnectedAnimation_.setPropertyName("opacityDisconnected");
    opacityDisconnectedAnimation_.setStartValue(1.0);
    opacityDisconnectedAnimation_.setEndValue(0.0);
    opacityDisconnectedAnimation_.setDuration(ANIMATION_DURATION);

    connect(&backgroundImage_, SIGNAL(updated()), SLOT(doUpdate()));

    topFrameBG_         = "background/WIN_MAIN_BG";
    headerDisconnected_ = "background/WIN_HEADER_BG_DISCONNECTED";
    headerConnected_    = "background/WIN_HEADER_BG_CONNECTED";
    headerConnecting_   = "background/WIN_HEADER_BG_CONNECTED"; // same as connected
    bottomFrameBG_      = "background/WIN_BOTTOM_BG";

    bottomLeftHorizDivider_ = "BOTTOMLEFT_HORIZ_DIVIDER_WHITE";

    midRightVertDivider_.reset(new ImageWithShadow("MIDRIGHT_VERT_DIVIDER", "MIDRIGHT_VERT_DIVIDER_SHADOW"));

#ifdef Q_OS_MAC
    topFrameBG_         = "background/MAC_MAIN_BG";
    headerDisconnected_ = "background/MAC_HEADER_BG_DISCONNECTED";
    headerConnected_    = "background/MAC_HEADER_BG_CONNECTED";
    headerConnecting_   = "background/MAC_HEADER_BG_CONNECTED"; // same as connected
    bottomFrameBG_      = "background/MAC_BOTTOM_BG";
#endif

}

QRectF Background::boundingRect() const
{
    return QRectF(0, 0, WIDTH * G_SCALE, HEIGHT * G_SCALE);
}

void Background::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // MAIN BG
    {
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(topFrameBG_);
        pixmap->draw(0, 0, painter);
    }

    // Background (FLAG, custom background, ...)
    {
        QPixmap *backgroundPixmap = backgroundImage_.currentPixmap();
        if (backgroundPixmap)
        {
            painter->setOpacity(1.0);
            painter->drawPixmap(0, 50*G_SCALE, *backgroundPixmap);
        }
    }

    // TOP GRADIENT
    {
        if (!qFuzzyIsNull(opacityConnecting_))
        {
            QPixmap *connectingPixmap = backgroundImage_.currentConnectingPixmap();
            if (connectingPixmap)
            {
                painter->setOpacity(opacityConnecting_);
                painter->drawPixmap(0, 0, *connectingPixmap);
            }
        }

        if (!qFuzzyIsNull(opacityConnected_))
        {
            QPixmap *connectedPixmap = backgroundImage_.currentConnectedPixmap();
            if (connectedPixmap)
            {
                painter->setOpacity(opacityConnected_);
                painter->drawPixmap(0, 0, *connectedPixmap);
            }
        }
    }

    // HEADER
    {
        if (!qFuzzyIsNull(opacityDisconnected_))
        {
            painter->setOpacity(opacityDisconnected_);
            QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(headerDisconnected_);
            pixmap->draw(0, 27*G_SCALE, painter);
        }
    }
    {
        if (!qFuzzyIsNull(opacityConnected_))
        {
            painter->setOpacity(opacityConnected_);
            QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(headerConnected_);
            pixmap->draw(0, 27*G_SCALE, painter);
        }
    }
    {
        if (!qFuzzyIsNull(opacityConnecting_))
        {
            painter->setOpacity(opacityConnecting_);
            QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(headerConnecting_);
            pixmap->draw(0, 27*G_SCALE, painter);
        }
    }

    // BOTTOM
    {
        painter->setOpacity(1);
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(bottomFrameBG_);
        pixmap->draw(0, 194*G_SCALE, painter);
    }

    // DIVIDER
    {
        midRightVertDivider_->draw(painter, boundingRect().width() - 54 * G_SCALE, 166 * G_SCALE);
    }
    {
        painter->setOpacity(0.1);
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(bottomLeftHorizDivider_);
        pixmap->draw(0, 248 * G_SCALE, painter);
    }
}

void Background::onConnectStateChanged(ProtoTypes::ConnectStateType newConnectState, ProtoTypes::ConnectStateType prevConnectState)
{
    Q_UNUSED(prevConnectState);

    if (newConnectState == ProtoTypes::CONNECTING || newConnectState == ProtoTypes::DISCONNECTING)
    {
        if (!qFuzzyCompare(opacityConnecting_, 1.0))
        {
            opacityConnectingAnimation_.setDirection(QPropertyAnimation::Forward);
            if (opacityConnectingAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectingAnimation_.start();
            }
        }
        else
        {
            if (opacityConnectingAnimation_.state() == QPropertyAnimation::Running)
                opacityConnectingAnimation_.stop();
        }

        if (!qFuzzyIsNull(opacityConnected_))
        {
            opacityConnectedAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectedAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectedAnimation_.start();
            }
        }
        else
        {
            if (opacityConnectedAnimation_.state() == QPropertyAnimation::Running)
                opacityConnectedAnimation_.stop();
        }
    }
    else if (newConnectState == ProtoTypes::CONNECTED)
    {
        if (!qFuzzyCompare(opacityConnected_, 1.0))
        {
            opacityConnectedAnimation_.setDirection(QPropertyAnimation::Forward);
            if (opacityConnectedAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectedAnimation_.start();
            }
        }
        else
        {
            if (opacityConnectedAnimation_.state() == QPropertyAnimation::Running)
                opacityConnectedAnimation_.stop();
        }

        if (!qFuzzyIsNull(opacityConnecting_))
        {
            opacityConnectingAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectingAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectingAnimation_.start();
            }
        }
        else
        {
            if (opacityConnectingAnimation_.state() == QPropertyAnimation::Running)
                opacityConnectingAnimation_.stop();
        }
    }
    else if (newConnectState == ProtoTypes::DISCONNECTED)
    {
        if (!qFuzzyIsNull(opacityConnecting_))
        {
            opacityConnectingAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectingAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectingAnimation_.start();
            }
        }
        else
        {
            if (opacityConnectingAnimation_.state() == QPropertyAnimation::Running)
                opacityConnectingAnimation_.stop();
        }

        if (!qFuzzyIsNull(opacityConnected_))
        {
            opacityConnectedAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectedAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectedAnimation_.start();
            }
        }
        else
        {
            if (opacityConnectedAnimation_.state() == QPropertyAnimation::Running)
                opacityConnectedAnimation_.stop();
        }
    }

    backgroundImage_.setIsConnected(newConnectState == ProtoTypes::CONNECTED);
}

void Background::onLocationSelected(const QString &countryCode)
{
    backgroundImage_.changeFlag(countryCode);
}

void Background::setDarkMode(bool dark)
{
    if (dark)
    {
        bottomLeftHorizDivider_ = "BOTTOMLEFT_HORIZ_DIVIDER_WHITE";
    }
    else
    {
        bottomLeftHorizDivider_ = "BOTTOMLEFT_HORIZ_DIVIDER";
    }

    update();
}

qreal Background::opacityConnecting()
{
    return opacityConnecting_;
}

void Background::setOpacityConnecting(qreal v)
{
    opacityConnecting_ = v;
    update();
}

qreal Background::opacityConnected()
{
     return opacityConnected_;
}

void Background::setOpacityConnected(qreal v)
{
    opacityConnected_ = v;
    update();
}

qreal Background::opacityDisconnected()
{
    return opacityDisconnected_;
}

void Background::setOpacityDisconnected(qreal v)
{
    opacityDisconnected_ = v;
    update();
}

QPixmap Background::getShadowPixmap()
{
    QPixmap tempPixmap(WIDTH*G_SCALE, HEIGHT*G_SCALE);
    tempPixmap.fill(Qt::transparent);

    QPainter painter(&tempPixmap);
    {
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(topFrameBG_);
        pixmap->draw(0, 0, &painter);
    }

    return tempPixmap;
}

void Background::updateScaling()
{
    backgroundImage_.updateScaling();
    midRightVertDivider_.reset(new ImageWithShadow("MIDRIGHT_VERT_DIVIDER", "MIDRIGHT_VERT_DIVIDER_SHADOW"));
    ScalableGraphicsObject::updateScaling();
}


void Background::doUpdate()
{
    update();
}

} //namespace ConnectWindow
