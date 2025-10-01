#pragma once

#include <QGraphicsObject>
#include "credentialswindowitem.h"
#include "loginwindowtypes.h"
#include "welcomewindowitem.h"

namespace LoginWindow {

// Basically item which contains two other items - CredentialsWindowItem and WelcomWindowItem
class LoginWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;
    void updateScaling() override;

    explicit LoginWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void setErrorMessage(ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage=QString());
    void setEmergencyConnectState(bool isEmergencyConnected);

    void resetState();

    void setClickable(bool enabled);
    void setCurrent2FACode(QString code);

    void transitionToUsernameScreen();
    void setFirewallTurnOffButtonVisibility(bool visible);

signals:
    void minimizeClick();
    void closeClick();
    void preferencesClick();
    void emergencyConnectClick();
    void externalConfigModeClick();
    void twoFactorAuthClick(const QString &username, const QString &password);
    void loginClick(const QString &username, const QString &password, const QString &code2fa);
    void haveAccountYesClick();
    void backToWelcomeClick();
    void firewallTurnOffClick();

protected:
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void updatePositions();
    void firewallTurnOffClickWelcomeWindow();
    void firewallTurnOffClickCredentialsWindow();

private:
    static constexpr int SCREEN_SWITCH_OPACITY_ANIMATION_DURATION = 150;

    bool isWelcomeScreen_;
    WelcomeWindowItem *welcomeWindowItem_;
    CredentialsWindowItem *credentialsWindowItem_;
};

} // namespace LoginWindow
