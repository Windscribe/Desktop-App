#include "loginwindowitem.h"

#include <QPainter>
#include "dpiscalemanager.h"

namespace LoginWindow {

LoginWindowItem::LoginWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), isWelcomeScreen_(true)
{
    Q_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    welcomeWindowItem_ = new WelcomeWindowItem(this, preferencesHelper);
    credentialsWindowItem_ = new CredentialsWindowItem(this, preferencesHelper);
    credentialsWindowItem_->setVisible(false);

    connect(welcomeWindowItem_, SIGNAL(minimizeClick()), SIGNAL(minimizeClick()));
    connect(welcomeWindowItem_, SIGNAL(closeClick()), SIGNAL(closeClick()));
    connect(welcomeWindowItem_, SIGNAL(preferencesClick()), SIGNAL(preferencesClick()));
    connect(welcomeWindowItem_, SIGNAL(emergencyConnectClick()), SIGNAL(emergencyConnectClick()));
    connect(welcomeWindowItem_, SIGNAL(externalConfigModeClick()), SIGNAL(externalConfigModeClick()));
    connect(welcomeWindowItem_, SIGNAL(haveAccountYesClick()), SIGNAL(haveAccountYesClick()));
    connect(welcomeWindowItem_, SIGNAL(firewallTurnOffClick()), SLOT(firewallTurnOffClickWelcomeWindow()));

    connect(credentialsWindowItem_, SIGNAL(minimizeClick()), SIGNAL(minimizeClick()));
    connect(credentialsWindowItem_, SIGNAL(closeClick()), SIGNAL(closeClick()));
    connect(credentialsWindowItem_, SIGNAL(preferencesClick()), SIGNAL(preferencesClick()));
    connect(credentialsWindowItem_, SIGNAL(emergencyConnectClick()), SIGNAL(emergencyConnectClick()));
    connect(credentialsWindowItem_, SIGNAL(externalConfigModeClick()), SIGNAL(externalConfigModeClick()));
    connect(credentialsWindowItem_, SIGNAL(backClick()), SIGNAL(backToWelcomeClick()));
    connect(credentialsWindowItem_, SIGNAL(twoFactorAuthClick(QString, QString)), SIGNAL(twoFactorAuthClick(QString, QString)));
    connect(credentialsWindowItem_, SIGNAL(loginClick(QString, QString, QString)), SIGNAL(loginClick(QString, QString, QString)));
    connect(credentialsWindowItem_, SIGNAL(firewallTurnOffClick()), SLOT(firewallTurnOffClickCredentialsWindow()));
}

QGraphicsObject *LoginWindowItem::getGraphicsObject()
{
    return this;
}

void LoginWindowItem::setErrorMessage(ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage)
{
    credentialsWindowItem_->setErrorMessage(errorMessageType, errorMessage);
}

void LoginWindowItem::setEmergencyConnectState(bool isEmergencyConnected)
{
    credentialsWindowItem_->setEmergencyConnectState(isEmergencyConnected);
    welcomeWindowItem_->setEmergencyConnectState(isEmergencyConnected);
}

QRectF LoginWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void LoginWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void LoginWindowItem::resetState()
{
    credentialsWindowItem_->resetState();
    welcomeWindowItem_->resetState();

    credentialsWindowItem_->stackBefore(welcomeWindowItem_);

    welcomeWindowItem_->setClickable(false);
    credentialsWindowItem_->setClickable(false);

    welcomeWindowItem_->setOpacity(0.0);
    welcomeWindowItem_->setVisible(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(welcomeWindowItem_);
    anim->setPropertyName("opacity");
    anim->setStartValue(welcomeWindowItem_->opacity());
    anim->setEndValue(1.0);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this]()
    {
        isWelcomeScreen_ = true;
        credentialsWindowItem_->setVisible(false);
        welcomeWindowItem_->setClickable(true);
        welcomeWindowItem_->setFocus();
        credentialsWindowItem_->setClickable(true);
    });

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void LoginWindowItem::setClickable(bool enabled)
{
    credentialsWindowItem_->setClickable(enabled);
    welcomeWindowItem_->setClickable(enabled);
}

void LoginWindowItem::setCurrent2FACode(QString code)
{
    credentialsWindowItem_->setCurrent2FACode(code);
}

void LoginWindowItem::updatePositions()
{
}

void LoginWindowItem::firewallTurnOffClickWelcomeWindow()
{
    credentialsWindowItem_->setFirewallTurnOffButtonVisibility(false);
    emit firewallTurnOffClick();
}

void LoginWindowItem::firewallTurnOffClickCredentialsWindow()
{
    welcomeWindowItem_->setFirewallTurnOffButtonVisibility(false);
    emit firewallTurnOffClick();
}

void LoginWindowItem::transitionToUsernameScreen()
{
    welcomeWindowItem_->stackBefore(credentialsWindowItem_);

    welcomeWindowItem_->setClickable(false);
    credentialsWindowItem_->setClickable(false);

    credentialsWindowItem_->setOpacity(0.0);
    credentialsWindowItem_->setVisible(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(credentialsWindowItem_);
    anim->setPropertyName("opacity");
    anim->setStartValue(credentialsWindowItem_->opacity());
    anim->setEndValue(1.0);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this]()
    {
        isWelcomeScreen_ = false;
        welcomeWindowItem_->setVisible(false);
        welcomeWindowItem_->setClickable(true);
        credentialsWindowItem_->setClickable(true);
        credentialsWindowItem_->setFocus();
        credentialsWindowItem_->setUsernameFocus();
    });

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void LoginWindowItem::setFirewallTurnOffButtonVisibility(bool visible)
{
    credentialsWindowItem_->setFirewallTurnOffButtonVisibility(visible);
    welcomeWindowItem_->setFirewallTurnOffButtonVisibility(visible);
}

void LoginWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
}

} // namespace LoginWindow

