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


Background::Background(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    opacityConnecting_(0), opacityConnected_(0), opacityDisconnected_(1), opacityCurFlag_(1.0), opacityPrevFlag_(0.0)
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

    opacityFlagAnimation_.setStartValue(0.0);
    opacityFlagAnimation_.setEndValue(1.0);
    opacityFlagAnimation_.setDuration(500);
    connect(&opacityFlagAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onFlagOpacityChanged(QVariant)));

    topFrameBG_         = "background/WIN_MAIN_BG";
    connectingGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTING";
    connectedGradient_  = "background/WIN_TOP_GRADIENT_BG_CONNECTED";
    headerDisconnected_ = "background/WIN_HEADER_BG_DISCONNECTED";
    headerConnected_    = "background/WIN_HEADER_BG_CONNECTED";
    headerConnecting_   = "background/WIN_HEADER_BG_CONNECTED"; // same as connected
    bottomFrameBG_      = "background/WIN_BOTTOM_BG";

    flagGradient_       = "background/FLAG_GRADIENT";
    midRightVertDivider_ = "MIDRIGHT_VERT_DIVIDER";
    bottomLeftHorizDivider_ = "BOTTOMLEFT_HORIZ_DIVIDER_WHITE";

#ifdef Q_OS_MAC
    topFrameBG_         = "background/MAC_MAIN_BG";
    connectingGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTING";
    connectedGradient_  = "background/MAC_TOP_GRADIENT_BG_CONNECTED";
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
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(topFrameBG_);
        pixmap->draw(0, 0, painter);
    }

    // FLAG
    {
        QPixmap pixmap(WIDTH * G_SCALE, 176 * G_SCALE);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);

        if (!prevCountryCode_.isEmpty())
        {
            IndependentPixmap *indPix = ImageResourcesSvg::instance().getScaledFlag(prevCountryCode_, WIDTH * G_SCALE, 176 * G_SCALE);
            p.setOpacity(opacityPrevFlag_);
            indPix->draw(0, 0, &p);
        }

        if (!countryCode_.isEmpty())
        {
            IndependentPixmap *indPix = ImageResourcesSvg::instance().getScaledFlag(countryCode_, WIDTH * G_SCALE, 176 * G_SCALE);
            p.setOpacity(opacityCurFlag_);
            indPix->draw(0, 0, &p);
        }

        painter->setOpacity(0.4);
        painter->drawPixmap(0, 50*G_SCALE, pixmap);
    }

    // FLAG GRADIENT
    {
        painter->setOpacity(1.0);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(flagGradient_);
        pixmap->draw(0, 50 * G_SCALE, painter);
    }

    // TOP GRADIENT
    {
        painter->setOpacity(opacityConnecting_);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(connectingGradient_);
        pixmap->draw(0, 0, painter);
    }
    {
        painter->setOpacity(opacityConnected_);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(connectedGradient_);
        pixmap->draw(0, 0, painter);
    }

    // HEADER
    {
        painter->setOpacity(opacityDisconnected_);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(headerDisconnected_);
        pixmap->draw(0, 27*G_SCALE, painter);
    }
    {
        painter->setOpacity(opacityConnected_);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(headerConnected_);
        pixmap->draw(0, 27*G_SCALE, painter);
    }
    {
        painter->setOpacity(opacityConnecting_);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(headerConnecting_);
        pixmap->draw(0, 27*G_SCALE, painter);
    }

    // BOTTOM
    {
        painter->setOpacity(1);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(bottomFrameBG_);
        pixmap->draw(0, 194*G_SCALE, painter);
    }

    // DIVIDERS
    {
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(midRightVertDivider_);
        pixmap->draw(boundingRect().width() - 54 * G_SCALE, 166 * G_SCALE, painter);
    }
    {
        painter->setOpacity(0.1);
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(bottomLeftHorizDivider_);
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

        if (!qFuzzyIsNull(opacityConnected_))
        {
            opacityConnectedAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectedAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectedAnimation_.start();
            }
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

        if (!qFuzzyIsNull(opacityConnecting_))
        {
            opacityConnectingAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectingAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectingAnimation_.start();
            }
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

        if (!qFuzzyIsNull(opacityConnected_))
        {
            opacityConnectedAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectedAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectedAnimation_.start();
            }
        }
    }
}

void Background::onLocationSelected(const QString &countryCode)
{
    if (countryCode_.isEmpty())
    {
        countryCode_ = countryCode;
        opacityPrevFlag_ = 0.0;
        opacityCurFlag_ = 1.0;
        update();
    }
    else if (countryCode_ != countryCode)
    {
        prevCountryCode_ = countryCode_;
        countryCode_ = countryCode;
        opacityPrevFlag_ = 1.0;
        opacityCurFlag_ = 0.0;
        opacityFlagAnimation_.start();
        update();
    }
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
        IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(topFrameBG_);
        pixmap->draw(0, 0, &painter);
    }

    return tempPixmap;
}


void Background::onFlagOpacityChanged(const QVariant &value)
{
    opacityPrevFlag_ = 1.0 - value.toDouble();
    opacityCurFlag_ = value.toDouble();
    update();
}

} //namespace ConnectWindow
