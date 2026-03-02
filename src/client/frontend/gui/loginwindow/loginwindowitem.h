#pragma once

#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include "backend/backend.h"
#include "loginwindowtypes.h"
#include "loginyesnobutton.h"
#include "usernamepasswordentry.h"
#include "commongraphics/iconbutton.h"
#include "loginbutton.h"
#include "firewallturnoffbutton.h"
#include "commongraphics/textbutton.h"
#include "iconhoverengagebutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "tooltips/tooltiptypes.h"

namespace LoginWindow {

class LoginWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit LoginWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void setErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage = QString());

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void resetState();

    void setClickable(bool enabled);
    void setCurrent2FACode(QString code);

    void setFirewallTurnOffButtonVisibility(bool visible);
    void updateScaling() override;

    void setUsernameFocus();

signals:
    void minimizeClick();
    void closeClick();
    void twoFactorAuthClick(const QString &username, const QString &password);
    void loginClick(const QString &username, const QString &password,
                    const QString &code2fa);
    void firewallTurnOffClick();
    void backClick();

private slots:
    void onBackClick();
    void onLoginClick();

    void onCloseClick();
    void onMinimizeClick();

    void onTwoFactorAuthClick();
    void onForgotPassClick();

    void onForgotAnd2FAPosYChanged(const QVariant &value);
    void onErrorChanged(const QVariant &value);

    void onEmergencyTextTransition(const QVariant &value);

    void onUsernamePasswordTextChanged(const QString &text);

    void onFirewallTurnOffClick();

    void onStandardLoginClick();
    void onHashedLoginClick();

    void onUploadFileClick();

    void onLanguageChanged();
    void onDockedModeChanged(bool bIsDockedToTray);
    void updatePositions();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private:
    void attemptLogin();

    void showEditBoxes();

    int centeredOffset(int background_length, int graphic_length);

    bool isUsernameAndPasswordValid() const;


    IconButton *minimizeButton_;
    IconButton *closeButton_;

    UsernamePasswordEntry *usernameEntry_;
    UsernamePasswordEntry *passwordEntry_;
    UsernamePasswordEntry *hashEntry_;

    double curEmergencyTextOpacity_;
    QVariantAnimation emergencyTextAnimation_;

    int curForgotAnd2FAPosY_;
    CommonGraphics::TextButton *twoFactorAuthButton_;
    CommonGraphics::TextButton *forgotPassButton_;
    QVariantAnimation forgotAnd2FAPosYAnimation_;

    LoginWindow::ERROR_MESSAGE_TYPE curError_;
    QString curErrorMsg_;
    QString curErrorText_;
    double curErrorOpacity_;
    QVariantAnimation errorAnimation_;

    IconButton *backButton_;
    LoginButton *loginButton_;
    FirewallTurnOffButton *firewallTurnOffButton_;

    QString current2FACode_;

    bool isHashedMode_ = false;
    CommonGraphics::TextButton *standardLoginButton_;
    CommonGraphics::TextButton *hashedLoginButton_;

    static constexpr int HEADER_HEIGHT               = 71;

    static constexpr int Y_COORD_YES_BUTTON          = 196;
    static constexpr int Y_COORD_NO_BUTTON           = 241;
    static constexpr int Y_COORD_YES_LINE            = 194;
    static constexpr int Y_COORD_NO_LINE             = 244;

    static constexpr int EMERGENCY_CONNECT_TEXT_WIDTH = 125;
    static constexpr int EMERGENCY_CONNECT_TEXT_POS_Y = 286;

    static constexpr int LOGIN_TEXT_HEIGHT    = 32;

    static constexpr int FORGOT_POS_X_SPACING = 24;

    static constexpr int LINE_EDIT_POS_Y_INVISIBLE = -50;
    static constexpr int USERNAME_POS_Y_VISIBLE = 92;
    static constexpr int PASSWORD_POS_Y_VISIBLE = 164;

    int getForgotAnd2FAPosYDefault() const;
    int getForgotAnd2FAPosYError() const;

};

} // namespace LoginWindow
