#ifndef WELCOMEWINDOWITEM_H
#define WELCOMEWINDOWITEM_H

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

class WelcomeWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit WelcomeWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void setEmergencyConnectState(bool isEmergencyConnected);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void resetState();

    void setClickable(bool enabled);

    void setFirewallTurnOffButtonVisibility(bool visible);
    void updateScaling();

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
    void showTooltip(TooltipInfo data);
    void hideTooltip(TooltipId id);
    void firewallTurnOffClick();

private slots:
    void onBackClick();

    void onCloseClick();
    void onMinimizeClick();

    void onGotoLoginButtonClick();
    void onGetStartedButtonClick();

    void onSettingsButtonClick();
    void onEmergencyButtonClick();
    void onConfigButtonClick();

    void onBadgePosXChanged(const QVariant &value);
    void onBadgePosYChanged(const QVariant &value);
    void onBadgeScaleChanged(const QVariant &value);

    void onLoginTextOpacityChanged(const QVariant &value);

    void onButtonLinePosChanged(const QVariant &value);

    void onErrorChanged(const QVariant &value);

    void onEmergencyTextTransition(const QVariant &value);

    void onHideYesNoTimerTick();

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
    void updateEmergencyConnect();
    void showYesNo();

    void transitionToEmergencyON();
    void transitionToEmergencyOFF();

    void changeToAccountScreen();

    int centeredOffset(int background_length, int graphic_length);

    double curBadgeScale_;
    int curBadgePosX_;
    int curBadgePosY_;
    QVariantAnimation badgePosXAnimation_;
    QVariantAnimation badgePosYAnimation_;
    QVariantAnimation badgeScaleAnimation_;

    double curLoginTextOpacity_;
    QVariantAnimation loginTextOpacityAnimation_;

    IconButton *minimizeButton_;
    IconButton *closeButton_;

    CommonGraphics::BubbleButtonBright *getStartedButton_;
    CommonGraphics::TextButton *gotoLoginButton_;

    IconButton *settingsButton_;
    IconButton *configButton_;
    IconHoverEngageButton *emergencyButton_;

    double curEmergencyTextOpacity_;
    QVariantAnimation emergencyTextAnimation_;

    int curForgotAnd2FAPosY_;
    QVariantAnimation forgotAnd2FAPosYAnimation_;

    QString curErrorText_;
    double curErrorOpacity_;
    QVariantAnimation errorAnimation_;

    FirewallTurnOffButton *firewallTurnOffButton_;

    int curButtonLineXPos_;
    QVariantAnimation buttonLinePosAnimation_;

    bool emergencyConnectOn_;

    static constexpr int HEADER_HEIGHT               = 71;

    static constexpr int Y_COORD_YES_BUTTON          = 196;
    static constexpr int Y_COORD_NO_BUTTON           = 241;
    static constexpr int Y_COORD_YES_LINE            = 194;
    static constexpr int Y_COORD_NO_LINE             = 244;

    static constexpr int EMERGENCY_CONNECT_TEXT_WIDTH = 125;
    static constexpr int EMERGENCY_CONNECT_TEXT_POS_Y = 282;

    static constexpr int BADGE_HEIGHT_BIG     = 40;
    static constexpr int BADGE_WIDTH_BIG      = 40;

    static constexpr int BADGE_SMALL_POS_X    = 45;
    static constexpr int BADGE_SMALL_POS_Y    = 54;

    static constexpr int LOGIN_TEXT_HEIGHT    = 32;

    static constexpr double BADGE_SCALE_SMALL = 0.6;
    static constexpr double BADGE_SCALE_LARGE = 1.0;

};

} // namespace LoginWindow


#endif // WELCOMEWINDOWITEM_H
