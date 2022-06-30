#ifndef CREDENTIALSWINDOWITEM_H
#define CREDENTIALSWINDOWITEM_H

#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include "../backend/backend.h"
#include "iloginwindow.h"
#include "loginyesnobutton.h"
#include "usernamepasswordentry.h"
#include "commongraphics/iconbutton.h"
#include "loginbutton.h"
#include "firewallturnoffbutton.h"
#include "commongraphics/textbutton.h"
#include "iconhoverengagebutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "tooltips/tooltiptypes.h"
#include "commongraphics/bubblebuttonbright.h"
#include "commongraphics/bubblebuttondark.h"

namespace LoginWindow {

class CredentialsWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit CredentialsWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void setErrorMessage(ILoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage);
    void setEmergencyConnectState(bool isEmergencyConnected);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void resetState();

    void setClickable(bool enabled);
    void setCurrent2FACode(QString code);

    void transitionToUsernameScreen();
    void setFirewallTurnOffButtonVisibility(bool visible);
    void updateScaling() override;

    void setUsernameFocus();

public slots:
    void onTooltipButtonHoverLeave();

signals:
    void minimizeClick();
    void closeClick();
    void preferencesClick();
    void emergencyConnectClick();
    void externalConfigModeClick();
    void twoFactorAuthClick(const QString &username, const QString &password);
    void loginClick(const QString &username, const QString &password,
                    const QString &code2fa);
    void haveAccountYesClick();
    void firewallTurnOffClick();
    void backClick();

private slots:
    void onBackClick();
    void onLoginClick();

    void onCloseClick();
    void onMinimizeClick();

    void onGotoLoginButtonClick();
    void onGetStartedButtonClick();

    void onTwoFactorAuthClick();
    void onForgotPassClick();

    void onSettingsButtonClick();
    void onEmergencyButtonClick();
    void onConfigButtonClick();

    void onLoginTextOpacityChanged(const QVariant &value);

    void onForgotAnd2FAPosYChanged(const QVariant &value);
    void onErrorChanged(const QVariant &value);

    void onEmergencyTextTransition(const QVariant &value);

    void onUsernamePasswordTextChanged(const QString &text);

    void onFirewallTurnOffClick();

    void onEmergencyHoverEnter();
    void onConfigHoverEnter();
    void onSettingsHoverEnter();
    void onAbstractButtonHoverEnter(QGraphicsObject *button, QString text);

    void onLanguageChanged();
    void onDockedModeChanged(bool bIsDockedToTray);
    void updatePositions();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private:
    void attemptLogin();
    void updateEmergencyConnect();
    void hideUsernamePassword();
    void showUsernamePassword();

    void transitionToEmergencyON();
    void transitionToEmergencyOFF();

    int centeredOffset(int background_length, int graphic_length);

    bool isUsernameAndPasswordValid() const;

    double curLoginTextOpacity_;
    QVariantAnimation loginTextOpacityAnimation_;

    IconButton *minimizeButton_;
    IconButton *closeButton_;

    UsernamePasswordEntry *usernameEntry_;
    UsernamePasswordEntry *passwordEntry_;

    IconButton *settingsButton_;
    IconButton *configButton_;
    IconHoverEngageButton *emergencyButton_;

    double curEmergencyTextOpacity_;
    QVariantAnimation emergencyTextAnimation_;

    int curForgotAnd2FAPosY_;
    CommonGraphics::TextButton *twoFactorAuthButton_;
    CommonGraphics::TextButton *forgotPassButton_;
    QVariantAnimation forgotAnd2FAPosYAnimation_;

    QString curErrorText_;
    double curErrorOpacity_;
    QVariantAnimation errorAnimation_;

    IconButton *backButton_;
    LoginButton *loginButton_;
    FirewallTurnOffButton *firewallTurnOffButton_;

    bool emergencyConnectOn_;

    QString current2FACode_;

    static constexpr int HEADER_HEIGHT               = 71;

    static constexpr int Y_COORD_YES_BUTTON          = 196;
    static constexpr int Y_COORD_NO_BUTTON           = 241;
    static constexpr int Y_COORD_YES_LINE            = 194;
    static constexpr int Y_COORD_NO_LINE             = 244;

    static constexpr int EMERGENCY_CONNECT_TEXT_WIDTH = 125;
    static constexpr int EMERGENCY_CONNECT_TEXT_POS_Y = 282;

    static constexpr int LOGIN_TEXT_HEIGHT    = 32;

    static constexpr int FORGOT_AND_2FA_POS_Y_DEFAULT = 240;
    static constexpr int FORGOT_AND_2FA_POS_Y_ERROR   = 280;
    static constexpr int FORGOT_POS_X_SPACING = 24;

    static constexpr int LINE_EDIT_POS_Y_INVISIBLE = -50;
    static constexpr int USERNAME_POS_Y_VISIBLE = 92;
    static constexpr int PASSWORD_POS_Y_VISIBLE = 164;

};

} // namespace LoginWindow


#endif // CREDENTIALSWINDOWITEM_H
