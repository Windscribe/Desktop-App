#ifndef LOGINWINDOWITEM_H
#define LOGINWINDOWITEM_H

#include <QGraphicsObject>
#include "iloginwindow.h"
#include "credentialswindowitem.h"
#include "welcomewindowitem.h"

namespace LoginWindow {

// Basically item which contains two other items - CredentialsWindowItem and WelcomWindowItem
class LoginWindowItem : public ScalableGraphicsObject, public ILoginWindow
{
    Q_OBJECT
    Q_INTERFACES(ILoginWindow)
public:
    explicit LoginWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    QGraphicsObject *getGraphicsObject() override;
    void setErrorMessage(ERROR_MESSAGE_TYPE errorMessage) override;
    void setEmergencyConnectState(bool isEmergencyConnected) override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void resetState() override;

    void setClickable(bool enabled) override;
    void setCurrent2FACode(QString code) override;

    void transitionToUsernameScreen() override;
    void setFirewallTurnOffButtonVisibility(bool visible) override;

    void updateScaling() override;

public slots:
    //void onTooltipButtonHoverLeave();

signals:
    void minimizeClick() override;
    void closeClick() override;
    void preferencesClick() override;
    void emergencyConnectClick() override;
    void externalConfigModeClick() override;
    void twoFactorAuthClick(const QString &username, const QString &password) override;
    void loginClick(const QString &username, const QString &password,
                    const QString &code2fa) override;
    void haveAccountYesClick() override;
    void backToWelcomeClick() override;
    void firewallTurnOffClick();

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


#endif // LOGINWINDOWITEM_H
