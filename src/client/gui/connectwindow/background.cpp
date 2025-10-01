#include "background.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QtGlobal>
#include <QElapsedTimer>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "utils/makecustomshadow.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {


Background::Background(ScalableGraphicsObject *parent, Preferences *preferences) : ScalableGraphicsObject(parent),
    preferences_(preferences), opacityConnecting_(0), opacityDisconnected_(1), backgroundImage_(this, preferences)
{
    opacityConnectingAnimation_.setTargetObject(this);
    opacityConnectingAnimation_.setPropertyName("opacityConnecting");
    opacityConnectingAnimation_.setStartValue(0.0);
    opacityConnectingAnimation_.setEndValue(1.0);
    opacityConnectingAnimation_.setDuration(ANIMATION_DURATION);

    opacityDisconnectedAnimation_.setTargetObject(this);
    opacityDisconnectedAnimation_.setPropertyName("opacityDisconnected");
    opacityDisconnectedAnimation_.setStartValue(1.0);
    opacityDisconnectedAnimation_.setEndValue(0.0);
    opacityDisconnectedAnimation_.setDuration(ANIMATION_DURATION);

    connect(&backgroundImage_, &BackgroundImage::updated, this, &Background::doUpdate);
}

QRectF Background::boundingRect() const
{
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        return QRectF(0, 0, WIDTH*G_SCALE, WINDOW_HEIGHT_VAN_GOGH*G_SCALE);
    } else {
        return QRectF(0, 0, WIDTH*G_SCALE, HEIGHT*G_SCALE);
    }
}

void Background::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // MAIN BG
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        painter->setPen(Qt::NoPen);
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);
    } else {
        QSharedPointer<IndependentPixmap> bg = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BG");
        bg->draw(0, 0, painter);
    }

    // Set the composition mode to SourceIn, so we can only draw the flag/custom background on top of the background
    painter->setCompositionMode(QPainter::CompositionMode_SourceIn);

    // Background (FLAG, custom background, ...)
    QPixmap *backgroundPixmap = backgroundImage_.currentPixmap();
    if (backgroundPixmap) {
        painter->setOpacity(1.0);
        painter->drawPixmap(0, preferences_->appSkin() == APP_SKIN_VAN_GOGH ? 0 : 16*G_SCALE, *backgroundPixmap);
    }

    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    // TOP GRADIENT
    if (!qFuzzyIsNull(opacityConnecting_)) {
        QPixmap *connectingPixmap = backgroundImage_.currentConnectingPixmap();
        if (connectingPixmap) {
            painter->setOpacity(opacityConnecting_);
            painter->drawPixmap(0, 0, *connectingPixmap);
        }
    }

    // HEADER
    painter->setOpacity(opacityDisconnected_);
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("background/HEADER_BG");
    pixmap->draw(0, preferences_->appSkin() == APP_SKIN_VAN_GOGH ? 0 : 16*G_SCALE, painter);

    // Draw border last, so it is not covered by other items
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        QSharedPointer<IndependentPixmap> borderInner = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_INNER_VAN_GOGH");
        borderInner->draw(0, 0, WINDOW_WIDTH*G_SCALE, WINDOW_HEIGHT_VAN_GOGH*G_SCALE, painter);
    } else {
        QSharedPointer<IndependentPixmap> borderInner = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_INNER");
        borderInner->draw(0, 0, WINDOW_WIDTH*G_SCALE, (WINDOW_HEIGHT-1)*G_SCALE, painter);
    }
}

void Background::onConnectStateChanged(CONNECT_STATE newConnectState, CONNECT_STATE prevConnectState)
{
    Q_UNUSED(prevConnectState);

    if (newConnectState == CONNECT_STATE_DISCONNECTING || newConnectState == CONNECT_STATE_CONNECTED) {
        if (!qFuzzyCompare(opacityConnecting_, 1.0)) {
            opacityConnectingAnimation_.setDirection(QPropertyAnimation::Forward);
            if (opacityConnectingAnimation_.state() != QPropertyAnimation::Running)
            {
                opacityConnectingAnimation_.start();
            }
        } else {
            if (opacityConnectingAnimation_.state() == QPropertyAnimation::Running) {
                opacityConnectingAnimation_.stop();
            }
        }
    } else if (newConnectState == CONNECT_STATE_DISCONNECTED) {
        if (!qFuzzyIsNull(opacityConnecting_)) {
            opacityConnectingAnimation_.setDirection(QPropertyAnimation::Backward);
            if (opacityConnectingAnimation_.state() != QPropertyAnimation::Running) {
                opacityConnectingAnimation_.start();
            }
        } else {
            if (opacityConnectingAnimation_.state() == QPropertyAnimation::Running) {
                opacityConnectingAnimation_.stop();
            }
        }
    }

    backgroundImage_.setIsConnected(newConnectState == CONNECT_STATE_CONNECTED);
}

void Background::onLocationSelected(const QString &countryCode)
{
    backgroundImage_.changeFlag(countryCode);
}

void Background::setDarkMode(bool dark)
{
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
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        painter.setPen(Qt::NoPen);
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter.fillPath(path, QColor(9, 15, 25));
        painter.setPen(Qt::SolidLine);
    } else {
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BG");
        pixmap->draw(0, 0, &painter);
    }

    return tempPixmap;
}

void Background::updateScaling()
{
    backgroundImage_.updateScaling();
    ScalableGraphicsObject::updateScaling();
}

void Background::doUpdate()
{
    update();
}

} //namespace ConnectWindow
