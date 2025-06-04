#include "emergencyconnectwindowitem.h"

#include <QKeyEvent>
#include <QPainter>
#include <QTimer>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "utils/ws_assert.h"

namespace EmergencyConnectWindow {

EmergencyConnectWindowItem::EmergencyConnectWindowItem(QGraphicsObject *parent,
                                                       PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), errorConnecting_(false)
{
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    curTitleOpacity_ = OPACITY_FULL;
    curDescriptionOpacity_ = OPACITY_FULL;
    curIconOpacity_ = OPACITY_FULL;

    curSpinnerPageOpacity_ = OPACITY_HIDDEN;
    curSubDescriptionTextOpacity_ = OPACITY_HIDDEN;

    curSpinnerRotation_ = 0;
    lastSpinnerRotationStart_ = 0;
    spinnerRotationAnimationActive_ = false;

    connectButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kOutline, 108, 40, 20);
    connect(connectButton_, &CommonGraphics::BubbleButton::clicked, this, &EmergencyConnectWindowItem::onConnectClicked);

    disconnectButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kBright, 128, 40, 20);
    connect(disconnectButton_, &CommonGraphics::BubbleButton::clicked, this, &EmergencyConnectWindowItem::onDisconnectClicked);

    escButton_ = new CommonGraphics::EscapeButton(this);
    connect(escButton_, &CommonGraphics::EscapeButton::clicked, this, &EmergencyConnectWindowItem::onEscClicked);

    connect(&titleOpacityAnimation_, &QVariantAnimation::valueChanged, this, &EmergencyConnectWindowItem::onTitleOpacityChange);
    connect(&descriptionOpacityAnimation_, &QVariantAnimation::valueChanged, this, &EmergencyConnectWindowItem::onDescriptionTransition);
    connect(&iconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &EmergencyConnectWindowItem::onIconOpacityChange);

    connect(&spinnerOpacityAnimation_, &QVariantAnimation::valueChanged, this, &EmergencyConnectWindowItem::onSpinnerTransition);
    connect(&subDescriptionTextOpacityAnimation_, &QVariantAnimation::valueChanged, this, &EmergencyConnectWindowItem::onSubDescriptionTextTransition);

    connect(&spinnerRotationAnimation_, &QVariantAnimation::valueChanged, this, &EmergencyConnectWindowItem::onSpinnerRotation);

    types::ConnectState defaultConnectState;
    setState(defaultConnectState);

    accessText_ = new TextLinkItem(this);
    updatePositions();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &EmergencyConnectWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QRectF EmergencyConnectWindowItem::boundingRect() const
{
    return QRectF(0, 0, LOGIN_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void EmergencyConnectWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // background
    QColor black = FontManager::instance().getMidnightColor();
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(boundingRect(), 9*G_SCALE, 9*G_SCALE);

    // Icon
    painter->setOpacity(curIconOpacity_*initialOpacity);
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("emergencyconnect/EMERGENCY_CONNECT_ICON");
    pixmap->draw((WINDOW_WIDTH/2 - 20)*G_SCALE, ICON_POS_Y*G_SCALE, painter);

    painter->setPen(QColor(255,255,255));

    // title
    painter->setOpacity(curTitleOpacity_*initialOpacity);
    painter->setFont(FontManager::instance().getFont(16, QFont::Bold, 100));
    QRectF titleRect(0, TITLE_POS_Y * G_SCALE, LOGIN_WIDTH * G_SCALE, LOGIN_HEIGHT * G_SCALE);
    painter->drawText(titleRect, Qt::AlignHCenter, tr("Emergency Connect"));

    // desc
    painter->save();
    painter->setOpacity(curDescriptionOpacity_*initialOpacity);
    painter->setFont(FontManager::instance().getFont(14, QFont::Normal, 100));

    QString descriptionText;
    int desiredLines = 1;
    if (errorConnecting_) {
        descriptionText = tr("Emergency connection failure. Try again?");
        desiredLines = 2;
    } else {
        descriptionText = tr("Can't access Windscribe.com or login into the app on your restrictive network? Connect to the emergency server that unblocks all of Windscribe.");
        desiredLines = 3;
    }

    QFontMetrics fm = painter->fontMetrics();
    int width = fm.horizontalAdvance(descriptionText)/desiredLines; // 3 lines
    if (width < DESCRIPTION_WIDTH_MIN*G_SCALE) width = DESCRIPTION_WIDTH_MIN*G_SCALE;
    else if (width > (LOGIN_WIDTH-50)*G_SCALE) width = (LOGIN_WIDTH-50)*G_SCALE;
    painter->drawText(CommonGraphics::centeredOffset(LOGIN_WIDTH*G_SCALE, width), DESCRIPTION_POS_Y*G_SCALE,
                      width, LOGIN_HEIGHT*G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, descriptionText);
    painter->restore();

    // Spinner Screen:
    painter->setOpacity(curSpinnerPageOpacity_*initialOpacity);

    // spinner
    painter->save();
    pixmap = ImageResourcesSvg::instance().getIndependentPixmap("emergencyconnect/BIG_SPINNER");
    painter->translate(WINDOW_WIDTH/2 * G_SCALE, (SPINNER_POS_Y* G_SCALE) +pixmap->height() / 2);
    painter->rotate(curSpinnerRotation_);
    pixmap->draw(-pixmap->width() / 2,-pixmap->height() / 2, painter);
    painter->restore();

    // Sub-Description
    painter->setOpacity(curSubDescriptionTextOpacity_*initialOpacity);
    QRectF connectingRect(0, CONNECTING_TEXT_POS_Y*G_SCALE, LOGIN_WIDTH*G_SCALE, CONNECTING_TEXT_HEIGHT*G_SCALE);
    painter->drawText(connectingRect, Qt::AlignHCenter | Qt::TextWordWrap, tr(curSubDescription_.toStdString().c_str()));
}

void EmergencyConnectWindowItem::startSpinnerAnimation()
{
    spinnerRotationAnimationActive_= true;

    lastSpinnerRotationStart_ = curSpinnerRotation_;

    curTargetRotation_ = curSpinnerRotation_ + TARGET_ROTATION;
    startAnAnimation(spinnerRotationAnimation_, curSpinnerRotation_, curTargetRotation_, ANIMATION_SPEED_VERY_SLOW );
}

void EmergencyConnectWindowItem::stopSpinnerAnimation()
{
    spinnerRotationAnimationActive_ = false;
    spinnerRotationAnimation_.stop();

}

void EmergencyConnectWindowItem::setState(types::ConnectState state)
{

    if (state.connectState == CONNECT_STATE_CONNECTED) {
        errorConnecting_ =  (state.connectError != NO_CONNECT_ERROR);

        stopSpinnerAnimation();
        descriptionOpacityAnimation_.stop();
        subDescriptionTextOpacityAnimation_.stop();
        spinnerOpacityAnimation_.stop();

        connectButton_->setVisible(false);
        connectButton_->quickHide();
        curSpinnerPageOpacity_ = OPACITY_HIDDEN;
        curSubDescriptionTextOpacity_ = OPACITY_HIDDEN;

        curDescriptionOpacity_ = OPACITY_FULL;
        disconnectButton_->quickShow();
        disconnectButton_->setVisible(true);
    } else if (state.connectState == CONNECT_STATE_DISCONNECTED) {
        errorConnecting_ =  (state.connectError != NO_CONNECT_ERROR);

        stopSpinnerAnimation();
        descriptionOpacityAnimation_.stop();
        subDescriptionTextOpacityAnimation_.stop();
        spinnerOpacityAnimation_.stop();

        disconnectButton_->setVisible(false);
        disconnectButton_->quickHide();
        curSpinnerPageOpacity_ = OPACITY_HIDDEN;
        curSubDescriptionTextOpacity_ = OPACITY_HIDDEN;

        curDescriptionOpacity_ = OPACITY_FULL;
        connectButton_->quickHide();
        connectButton_->quickShow();
        connectButton_->setVisible(true);
    } else if (state.connectState == CONNECT_STATE_CONNECTING) {
        curSubDescription_ = tr("Connecting...");

        connectButton_->setVisible(false);
        disconnectButton_->setVisible(false);

        transitionToConnecting();
        startSpinnerAnimation();
    } else { // CONNECT_STATE_DISCONNECTING
        curSubDescription_ = tr("Disconnecting...");

        transitionToDisconnecting();
        startSpinnerAnimation();
    }

    state_ = state;

    update();
}

void EmergencyConnectWindowItem::setClickable(bool isClickable)
{
    if (state_.connectState == CONNECT_STATE_CONNECTED) {
        disconnectButton_->setClickable(isClickable);
    } else {
        disconnectButton_->setClickable(false);
    }

    if (state_.connectState == CONNECT_STATE_DISCONNECTED) {
        connectButton_->setClickable(isClickable);
    } else {
        connectButton_->setClickable(false);
    }

    escButton_->setClickable(isClickable);
}

void EmergencyConnectWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void EmergencyConnectWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (state_.connectState == CONNECT_STATE_CONNECTING) {
            emit disconnectClick();
        } else {
            emit escapeClick();
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (state_.connectState == CONNECT_STATE_CONNECTED) {
            emit disconnectClick();
        } else if (state_.connectState == CONNECT_STATE_DISCONNECTED) {
            emit connectClick();
        }
    }
}

void EmergencyConnectWindowItem::onEscClicked()
{
    if (state_.connectState == CONNECT_STATE_CONNECTING) {
        emit disconnectClick();
    }
    emit escapeClick();
}

void EmergencyConnectWindowItem::transitionToConnecting()
{
    curSpinnerRotation_ = 0;

    startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation(spinnerOpacityAnimation_, curSpinnerPageOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation(subDescriptionTextOpacityAnimation_, curSubDescriptionTextOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);

    connectButton_->animateHide(ANIMATION_SPEED_FAST);
    disconnectButton_->animateHide(ANIMATION_SPEED_FAST);
}

void EmergencyConnectWindowItem::transitionToConnect()
{
    startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation(spinnerOpacityAnimation_, curSpinnerPageOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation(subDescriptionTextOpacityAnimation_, curSubDescriptionTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);

    connectButton_->animateShow(ANIMATION_SPEED_FAST);
}

void EmergencyConnectWindowItem::transitionToDisconnecting()
{
    curSpinnerRotation_ = 0;

    startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation(spinnerOpacityAnimation_, curSpinnerPageOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation(subDescriptionTextOpacityAnimation_, curSubDescriptionTextOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);

    disconnectButton_->animateHide(ANIMATION_SPEED_FAST);
    connectButton_->animateHide(ANIMATION_SPEED_FAST);
}

void EmergencyConnectWindowItem::transitionToDisconnect()
{
    startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation(spinnerOpacityAnimation_, curSpinnerPageOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation(subDescriptionTextOpacityAnimation_, curSubDescriptionTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);

    disconnectButton_->animateShow(ANIMATION_SPEED_FAST);
}

void EmergencyConnectWindowItem::updatePositions()
{
    connectButton_->setPos(WINDOW_WIDTH/2*G_SCALE - connectButton_->boundingRect().width()/2, CONNECT_BUTTON_POS_Y*G_SCALE);
    disconnectButton_->setPos(WINDOW_WIDTH/2*G_SCALE - disconnectButton_->boundingRect().width()/2, DISCONNECT_BUTTON_POS_Y*G_SCALE);
    escButton_->setPos(WINDOW_WIDTH*G_SCALE - escButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE, WINDOW_MARGIN*G_SCALE);
    accessText_->setFixedWidth(WINDOW_WIDTH*G_SCALE);
    accessText_->move(0, LINK_TEXT_POS_Y*G_SCALE);
}

void EmergencyConnectWindowItem::onConnectClicked()
{
    emit connectClick();
}

void EmergencyConnectWindowItem::onDisconnectClicked()
{
    emit disconnectClick();
}

void EmergencyConnectWindowItem::onSpinnerTransition(const QVariant &value)
{
    curSpinnerPageOpacity_ = value.toDouble();
    update();
}

void EmergencyConnectWindowItem::onDescriptionTransition(const QVariant &value)
{
    curDescriptionOpacity_ = value.toDouble();
    update();
}

void EmergencyConnectWindowItem::onIconOpacityChange(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}

void EmergencyConnectWindowItem::onSubDescriptionTextTransition(const QVariant &value)
{
    curSubDescriptionTextOpacity_ = value.toDouble();
    update();
}

void EmergencyConnectWindowItem::onSpinnerRotation(const QVariant &value)
{
    curSpinnerRotation_ = value.toInt();

    // restart until stopped
    int diff = abs(curSpinnerRotation_ - curTargetRotation_);
    if (spinnerRotationAnimationActive_ &&  diff < 5) { //  close enough
        curSpinnerRotation_ = lastSpinnerRotationStart_ % TARGET_ROTATION; // prevents overflow and allows smooth restarts
        startSpinnerAnimation();
    }

    update();
}

void EmergencyConnectWindowItem::onLanguageChanged()
{
    connectButton_->setText(tr("Connect"));
    disconnectButton_->setText(tr("Disconnect"));
}

void EmergencyConnectWindowItem::onTitleOpacityChange(const QVariant &value)
{
    curTitleOpacity_ = value.toDouble();
    update();
}

}
