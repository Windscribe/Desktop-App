#ifndef LOGINWINDOWITEM_H
#define LOGINWINDOWITEM_H

#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QTimer>
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

namespace LoginWindow {

class LoginWindowItem : public ScalableGraphicsObject, public ILoginWindow
{
    Q_OBJECT
    Q_INTERFACES(ILoginWindow)
public:
    explicit LoginWindowItem(QGraphicsObject *parent = nullptr);

    QGraphicsObject *getGraphicsObject() override;
    void setErrorMessage(ERROR_MESSAGE_TYPE errorMessage) override;
    void setEmergencyConnectState(bool isEmergencyConnected) override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void resetState() override;

    void setClickable(bool enabled) override;
    void updateEmergencyConnect();

    void hideUsernamePassword();
    void showUsernamePassword();
    void showYesNo();

    void transitionToUsernameScreen() override;
    void setFirewallTurnOffButtonVisibility(bool visible) override;
    void updateScaling() override;

public slots:
    void onTooltipButtonHoverLeave();

signals:
    void minimizeClick() override;
    void closeClick() override;
    void preferencesClick() override;
    void emergencyConnectClick() override;
    void externalConfigModeClick() override;
    void loginClick(const QString &username, const QString &password) override;
    void haveAccountYesClick() override;
    void showTooltip(TooltipInfo data);
    void hideTooltip(TooltipId id);
    void firewallTurnOffClick();

private slots:
    void onBackClick();
    void onLoginClick();

    void onCloseClick();
    void onMinimizeClick();

    void onYesButtonClick();
    void onNoButtonClick();

    void onYesActivated();
    void onNoActivated();

    void onYesDeactivated();
    void onNoDeactivated();

    void onUsernameActivated();
    void onPasswordActivated();
    void onForgotPassClick();

    void onSettingsButtonClick();
    void onEmergencyButtonClick();
    void onConfigButtonClick();

    void onBadgePosXChanged(const QVariant &value);
    void onBadgePosYChanged(const QVariant &value);
    void onChildItemOpacityChanged(const QVariant &value);
    void onBadgeScaleChanged(const QVariant &value);

    void onLoginTextOpacityChanged(const QVariant &value);

    void onButtonLinePosChanged(const QVariant &value);

    void onForgotPosYChanged(const QVariant &value);
    void onErrorChanged(const QVariant &value);

    void onEmergencyTextTransition(const QVariant &value);

    void onHideYesNoTimerTick();

    void onUsernamePasswordKeyPress(QKeyEvent *event);

    void onFirewallTurnOffClick();

    void onEmergencyHoverEnter();
    void onConfigHoverEnter();
    void onSettingsHoverEnter();
    void onAbstractButtonHoverEnter(QGraphicsObject *button, QString text);

    void onLanguageChanged();
    void updatePositions();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private:
    void attemptLogin();

    void transitionToEmergencyON();
    void transitionToEmergencyOFF();

    void changeToAccountScreen();

    int centeredOffset(int background_length, int graphic_length);

    bool account_screen_;

    double curBadgeScale_;
    int curBadgePosX_;
    int curBadgePosY_;
    double curChildItemOpacity_;
    QVariantAnimation badgePosXAnimation_;
    QVariantAnimation badgePosYAnimation_;
    QVariantAnimation childItemOpacityAnimation_;
    QVariantAnimation badgeScaleAnimation_;

    double curLoginTextOpacity_;
    QVariantAnimation loginTextOpacityAnimation_;

    IconButton *minimizeButton_;
    IconButton *closeButton_;

    LoginYesNoButton *yesButton_;
    LoginYesNoButton *noButton_;

    UsernamePasswordEntry *usernameEntry_;
    UsernamePasswordEntry *passwordEntry_;

    IconButton *settingsButton_;
    IconButton *configButton_;
    IconHoverEngageButton *emergencyButton_;

    double curEmergencyTextOpacity_;
    QVariantAnimation emergencyTextAnimation_;

    int curForgotPosY_;
    CommonGraphics::TextButton *forgotPassButton_;
    QVariantAnimation forgotPosYAnimation_;

    QString curErrorText_;
    double curErrorOpacity_;
    QVariantAnimation errorAnimation_;

    IconButton *backButton_;
    LoginButton *loginButton_;
    FirewallTurnOffButton *firewallTurnOffButton_;

    QGraphicsPixmapItem *shadowBackgroundPixmap;

    int curButtonLineXPos_;
    QVariantAnimation buttonLinePosAnimation_;

    bool emergencyConnectOn_;

    static constexpr int HEADER_HEIGHT               = 71;

    static constexpr int Y_COORD_YES_BUTTON          = 160;
    static constexpr int Y_COORD_NO_BUTTON           = 210;
    static constexpr int Y_COORD_YES_LINE            = 194;
    static constexpr int Y_COORD_NO_LINE             = 244;

    static constexpr int EMERGENCY_CONNECT_TEXT_WIDTH = 125;
    static constexpr int EMERGENCY_CONNECT_TEXT_POS_Y = 282;

    static constexpr int BADGE_HEIGHT_BIG     = 40;
    static constexpr int BADGE_WIDTH_BIG      = 40;

    static constexpr int BADGE_SMALL_POS_X    = 45;
    static constexpr int BADGE_SMALL_POS_Y    = 54;

    static constexpr int LOGIN_TEXT_HEIGHT    = 32;

    static constexpr int FORGOT_POS_Y_DEFAULT = 240;
    static constexpr int FORGOT_POS_Y_ERROR   = 280;

    static constexpr double BADGE_SCALE_SMALL = 0.6;
    static constexpr double BADGE_SCALE_LARGE = 1.0;

    static constexpr int LINE_EDIT_POS_Y_INVISIBLE = -50;
    static constexpr int USERNAME_POS_Y_VISIBLE = 92;
    static constexpr int PASSWORD_POS_Y_VISIBLE = 164;

};

} // namespace LoginWindow


#endif // LOGINWINDOWITEM_H
