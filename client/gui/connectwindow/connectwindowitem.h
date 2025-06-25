#pragma once

#include <QGraphicsObject>
#include <QTimer>

#include "background.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/textbutton.h"
#include "commongraphics/togglebutton.h"
#include "connectbutton.h"
#include "connectstateprotocolport/connectstateprotocolport.h"
#include "ipaddressitem/ipaddressitem.h"
#include "locationsbutton.h"
#include "logonotificationsbutton.h"

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

    void setConnectionTimeAndData(QString connectionTime, QString dataTransferred);
    void setFirewallAlwaysOn(bool isFirewallAlwaysOn);
    void setTestTunnelResult(bool success);
    types::ProtocolStatus getProtocolStatus();
    void setIsPreferredProtocol(bool on);
    void setIsPremium(bool isPremium);
    void setCustomConfigMode(bool isCustomConfig);

    bool handleKeyPressEvent(QKeyEvent *event);

public slots:
    void updateLocationInfo(const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isCustomConfig);
    void updateConnectState(const types::ConnectState & newConnectState);
    void updateFirewallState(bool isFirewallEnabled);
    void updateLocationsState(bool isExpanded);
    void updateMyIp(const QString &ip);
    void updateNotificationsState(int totalMessages, int unread);
    void updateNetworkState(types::NetworkInterface network);
    void setSplitTunnelingState(bool on);
    void setInternetConnectivity(bool connectivity);
    void setProtocolPort(const types::Protocol &protocol, const uint port);

    void onNetworkHoverEnter();
    void onNetworkHoverLeave();
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

    void locationTabClicked(LOCATION_TAB tab);
    void searchFilterChanged(const QString &filter);
    void locationsKeyPressed(QKeyEvent *event);

private slots:
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
    IconButton *networkTrustButton_;
    IconButton *dotMenuButton_;
    IPAddressItem *ipAddressItem_;
    CommonGraphics::TextButton *firewallLabel_;
    CommonGraphics::TextButton *networkNameText_;
    LogoNotificationsButton *logoButton_;

    types::ConnectState prevConnectState_;
    NETWORK_TRUST_TYPE trustType_;
    NETWORK_INTERFACE_TYPE interfaceType_;
    bool networkActive_;

    QString fullFirstName_;
    QString fullSecondName_;
    QString networkName_;

    QString connectionTime_;
    QString dataTransferred_;

    bool isFirewallAlwaysOn_;

    void updatePositions();
    void updateFirewallInfo();
    void updateShortenedText();
};

} //namespace ConnectWindow
