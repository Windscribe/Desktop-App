#pragma once

#include <QGraphicsObject>
#include <QTimer>

#include "background.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/textbutton.h"
#include "connectbutton.h"
#include "connectstateprotocolport/connectstateprotocolport.h"
#include "firewallbutton.h"
#include "iconnectwindow.h"
#include "locationsbutton.h"
#include "logonotificationsbutton.h"
#include "middleitem.h"
#include "serverratingindicator.h"

namespace ConnectWindow {

class ConnectWindowItem : public ScalableGraphicsObject, public IConnectWindow
{
    Q_OBJECT
    Q_INTERFACES(IConnectWindow)
public:
    explicit ConnectWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QGraphicsObject *getGraphicsObject() override;

    void setClickable(bool isClickable) override;

    QRegion getMask() override;
    QPixmap getShadowPixmap() override;

    void setConnectionTimeAndData(QString connectionTime, QString dataTransferred) override;
    void setFirewallAlwaysOn(bool isFirewallAlwaysOn) override;
    void setFirewallBlock(bool isFirewallBlocked) override;
    void setTestTunnelResult(bool success) override;
    void setCornerColor(QColor color) override;
    types::ProtocolStatus getProtocolStatus() override;
    void setIsPreferredProtocol(bool on) override;

    void updateScaling() override;

public slots:
    void updateLocationInfo(const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isCustomConfig) override;
    void updateConnectState(const types::ConnectState & newConnectState) override;
    void updateFirewallState(bool isFirewallEnabled) override;
    void updateLocationsState(bool isExpanded) override;
    void updateMyIp(const QString &ip) override;
    void updateNotificationsState(int totalMessages, int unread) override;
    void updateNetworkState(types::NetworkInterface network) override;
    void setSplitTunnelingState(bool on) override;
    void setInternetConnectivity(bool connectivity) override;
    void setProtocolPort(const types::Protocol &protocol, const uint port) override;

    void onNetworkHoverEnter();
    void onNetworkHoverLeave();
    void onConnectStateTextHoverEnter();
    void onConnectStateTextHoverLeave();
    void onFirewallButtonClick();
    void onFirewallButtonHoverEnter();
    void onFirewallButtonHoverLeave();
    void onFirewallInfoHoverEnter();
    void onFirewallInfoHoverLeave();
    void onFirstNameHoverEnter();
    void onSecondNameHoverEnter();
    void onFirstOrSecondNameHoverLeave();
    void onServerRatingIndicatorHoverEnter();
    void onServerRatingIndicatorHoverLeave();
    void onDockedModeChanged(bool bIsDockedToTray);
    void onProtocolsClick();

signals:
    void minimizeClick() override;
    void closeClick() override;
    void preferencesClick() override;
    void connectClick() override;
    void firewallClick() override;
    void locationsClick() override;
    void notificationsClick() override;
    void networkButtonClick() override;
    void splitTunnelingButtonClick() override;
    void protocolsClick() override;

private slots:
    void onAppSkinChanged(APP_SKIN s);
    void onLanguageChanged();

private:
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;
    IconButton *minimizeButton_;
    IconButton *closeButton_;
#ifdef Q_OS_MAC
    //IconButton *fullScreenButton_;
#endif

    Background *background_;
    ConnectButton *connectButton_;
    ConnectStateProtocolPort *connectStateProtocolPort_;
    CommonGraphics::TextButton *cityName1Text_;
    CommonGraphics::TextButton *cityName2Text_;
    LocationsButton *locationsButton_;
    ServerRatingIndicator *serverRatingIndicator_;
    MiddleItem *middleItem_;
    FirewallButton *firewallButton_;
    IconButton *firewallInfo_;
    IconButton *preferencesButton_;
    IconButton *networkTrustButton_;

    LogoNotificationsButton *logoButton_;

    types::ConnectState prevConnectState_;
    QString networkName_;
    NETWORK_TRUST_TYPE trustType_;
    NETWORK_INTERACE_TYPE interfaceType_;
    bool networkActive_;

    QString fullFirstName_;
    QString fullSecondName_;

    QString connectionTime_;
    QString dataTransferred_;

    bool isFirewallAlwaysOn_;
    bool isFirewallBlocked_;

    void updatePositions();
    void updateShortenedText();
};

} //namespace ConnectWindow
