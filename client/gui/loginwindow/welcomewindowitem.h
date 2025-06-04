#pragma once

#include <QGraphicsObject>
#include <QPixmap>
#include <QTimer>
#include "backend/backend.h"
#include "loginyesnobutton.h"
#include "usernamepasswordentry.h"
#include "commongraphics/iconbutton.h"
#include "loginbutton.h"
#include "firewallturnoffbutton.h"
#include "commongraphics/textbutton.h"
#include "iconhoverengagebutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "tooltips/tooltiptypes.h"
#include "commongraphics/bubblebutton.h"

namespace LoginWindow {

class WelcomeWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit WelcomeWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void setEmergencyConnectState(bool isEmergencyConnected);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setClickable(bool enabled);

    void setFirewallTurnOffButtonVisibility(bool visible);
    void updateScaling() override;

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

private slots:
    void onCloseClick();
    void onMinimizeClick();

    void onGotoLoginButtonClick();
    void onGetStartedButtonClick();

    void onSettingsButtonClick();
    void onEmergencyButtonClick();
    void onConfigButtonClick();

    void onEmergencyTextTransition(const QVariant &value);

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

private:
    void updateEmergencyConnect();
    void transitionToEmergencyON();
    void transitionToEmergencyOFF();
    int centeredOffset(int background_length, int graphic_length);

    IconButton *minimizeButton_;
    IconButton *closeButton_;

    CommonGraphics::BubbleButton *getStartedButton_;
    CommonGraphics::BubbleButton *gotoLoginButton_;

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
    bool emergencyConnectOn_;

    static constexpr int GET_STARTED_BUTTON_POS_Y = 136;
    static constexpr int LOGIN_BUTTON_POS_Y       = 186;
    static constexpr int LOGO_HEIGHT = 19;
    static constexpr int LOGO_WIDTH = 123;
    static constexpr int LOGO_POS_Y = 24;
};

} // namespace LoginWindow
