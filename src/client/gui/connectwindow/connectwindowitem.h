#pragma once

#include <QGraphicsObject>
#include <QTimer>
#include <QVariantAnimation>

#include "background.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/imageitem.h"
#include "commongraphics/textbutton.h"
#include "commongraphics/togglebutton.h"
#include "connectbutton.h"
#include "connectstateprotocolport/connectstateprotocolport.h"
#include "ipaddressitem/ipaddressitem.h"
#include "iputilsmenu.h"
#include "locationsbutton.h"
#include "logonotificationsbutton.h"
#include "networktrustbutton.h"

namespace ConnectWindow {

class ConnectWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ConnectWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setClickable(bool isClickable);

    QRegion getMask();
    QPixmap getShadowPixmap();

    QString getIpAddress();
    void setConnectionTimeAndData(QString connectionTime, QString dataTransferred);
    void setFirewallAlwaysOn(bool isFirewallAlwaysOn);
    void setTestTunnelResult(bool success);
    types::ProtocolStatus getProtocolStatus();
    void setIsPreferredProtocol(bool on);
    void setIsPremium(bool isPremium);
    void setCustomConfigMode(bool isCustomConfig);
    void setIpUtilsEnabled(bool enabled);
    void hideMenus();

    bool handleKeyPressEvent(QKeyEvent *event);

public slots:
    void updateLocationInfo(const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isCustomConfig);
    void updateConnectState(const types::ConnectState & newConnectState);
    void updateFirewallState(bool isFirewallEnabled);
    void updateLocationsState(bool isExpanded);
    void updateMyIp(const QString &ip, bool isPinned = false);
    void updateNotificationsState(int totalMessages, int unread);
    void updateNetworkState(types::NetworkInterface network);
    void setSplitTunnelingState(bool on);
    void setInternetConnectivity(bool connectivity);
    void setProtocolPort(const types::Protocol &protocol, const uint port);
    void onIpRotateResult(bool success);

    void onConnectStateTextHoverEnter();
    void onConnectStateTextHoverLeave();
    void onFirewallButtonHoverLeave();
    void onFirewallButtonToggleIgnored();
    void onFirewallButtonStateChanged(bool isEnabled);
    void onFirewallInfoHoverEnter();
    void onFirewallInfoHoverLeave();
    void onFirstNameHoverEnter();
    void onSecondNameHoverEnter();
    void onFirstOrSecondNameHoverLeave();
    void onDockedModeChanged(bool bIsDockedToTray);
    void onProtocolsClick();
    void onIpAddressWidthChanged(int width);
    void onLocationsCollapsed();
    void onDotMenuButtonClick();
    void onIpUtilsMenuFavouriteClick();
    void onIpUtilsMenuRotateClick();
    void onIpUtilsMenuCloseClick();
    void onIpUtilsMenuFavouriteHoverEnter();
    void onIpUtilsMenuFavouriteHoverLeave();
    void onIpUtilsMenuRotateHoverEnter();
    void onIpUtilsMenuRotateHoverLeave();
    void onIpUtilsMenuAnimationValueChanged(const QVariant &value);
    void onIpUtilsMenuAnimationFinished();
    void onFavouriteAnimationValueChanged(const QVariant &value);
    void onFavouriteAnimationFinished();

signals:
    void minimizeClick();
    void closeClick();
    void preferencesClick();
    void connectClick();
    void firewallClick();
    void locationsClick();
    void notificationsClick();
    void networkButtonClick();
    void splitTunnelingButtonClick();
    void protocolsClick();
    void rotateIpClick();
    void pinIp(const QString &ip);
    void unpinIp(const QString &ip);

    void locationTabClicked(LOCATION_TAB tab);
    void searchFilterChanged(const QString &filter);
    void locationsKeyPressed(QKeyEvent *event);

private slots:
    void onExternalConfigModeChanged(bool isExternalConfigMode);
    void onAppSkinChanged(APP_SKIN s);
    void onLanguageChanged();

private:
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;
    IconButton *minimizeButton_;
    IconButton *closeButton_;
#ifdef Q_OS_MACOS
    //IconButton *fullScreenButton_;
#endif

    Background *background_;
    ConnectButton *connectButton_;
    ConnectStateProtocolPort *connectStateProtocolPort_;
    CommonGraphics::TextButton *cityName1Text_;
    CommonGraphics::TextButton *cityName2Text_;
    LocationsButton *locationsButton_;
    ToggleButton *firewallButton_;
    IconButton *firewallInfo_;
    IconButton *preferencesButton_;
    NetworkTrustButton *networkTrustButton_;
    IconButton *dotMenuButton_;
    IpUtilsMenu *ipUtilsMenu_;
    QVariantAnimation *ipUtilsMenuAnimation_;
    ImageItem *favouriteAnimationIcon_;
    QVariantAnimation *favouriteAnimation_;
    IPAddressItem *ipAddressItem_;
    CommonGraphics::TextButton *firewallLabel_;
    LogoNotificationsButton *logoButton_;
    QTimer *rotateButtonReenableTimer_;

    types::ConnectState prevConnectState_;

    QString fullFirstName_;
    QString fullSecondName_;

    QString connectionTime_;
    QString dataTransferred_;

    bool isFirewallAlwaysOn_;
    bool isPremium_;
    bool isIpUtilsEnabled_;
    bool isIpPinned_;

    void updatePositions();
    void updateFirewallInfo();
    void updateShortenedText();
    void showIpUtilsMenu(bool animate);
    void hideIpUtilsMenu(bool animate);
};

} //namespace ConnectWindow
