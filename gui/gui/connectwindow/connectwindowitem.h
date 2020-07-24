#ifndef CONNECTWINDOWITEM_H
#define CONNECTWINDOWITEM_H

#include <QGraphicsObject>
#include <QTimer>
#include "background.h"
#include "../backend/backend.h"
#include "connectbutton.h"
#include "connectstateprotocolport.h"
#include "locationsbutton.h"
#include "serverratingindicator.h"
#include "wifiname.h"
#include "middleitem.h"
#include "firewallbutton.h"
#include "iconnectwindow.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/textbutton.h"
#include "commonwidgets/iconbuttonwidget.h"
#include "logonotificationsbutton.h"

namespace ConnectWindow {

class ConnectWindowItem : public ScalableGraphicsObject, public IConnectWindow
{
    Q_OBJECT
    Q_INTERFACES(IConnectWindow)
public:
    explicit ConnectWindowItem(QGraphicsObject  *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual QGraphicsObject *getGraphicsObject();

    virtual void setClickable(bool isClickable);

    virtual QRegion getMask();
    virtual QPixmap getShadowPixmap();

    virtual void setConnectionTimeAndData(QString connectionTime, QString dataTransferred);
    virtual void setFirewallAlwaysOn(bool isFirewallAlwaysOn);
    virtual void setFirewallBlock(bool isFirewallBlocked);
    void setTestTunnelResult(bool success);

    virtual void updateScaling();

public slots:
    virtual void updateLocationInfo(LocationID id, const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isFavorite);
    virtual void updateLocationSpeed(LocationID id, PingTime speed);
    virtual void updateConnectState(const ProtoTypes::ConnectState & newConnectState);
    virtual void updateFirewallState(bool isFirewallEnabled);
    virtual void updateLocationsState(bool isExpanded);
    virtual void updateFavoriteState(LocationID id, bool isFavorite);
    virtual void updateMyIp(const QString &ip);
    virtual void updateNotificationsState(int totalMessages, int unread);
    void updateNetworkState(ProtoTypes::NetworkInterface network);
    void setSplitTunnelingState(bool on);
    void setInternetConnectivity(bool connectivity);
    void setProtocolPort(const ProtoTypes::Protocol &protocol, const uint port);

    void onNetworkHoverEnter();
    void onNetworkHoverLeave();
    void onConnectStateTextHoverEnter();
    void onConnectStateTextHoverLeave();
    void onFirewallInfoHoverEnter();
    void onFirewallInfoHoverLeave();
    void onSecondNameHoverEnter();
    void onSecondNameHoverLeave();
    void onServerRatingIndicatorHoverEnter();
    void onServerRatingIndicatorHoverLeave();

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
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId type);

private:
    IconButton *minimizeButton_;
    IconButton *closeButton_;
#ifdef Q_OS_MAC
    //IconButton *fullScreenButton_;
#endif

    Background *background_;
    ConnectButton *connectButton_;
    ConnectStateProtocolPort *connectStateProtocolPort_;
    QGraphicsSimpleTextItem *cityName1Text_;
    CommonGraphics::TextButton *cityName2Text_;
    LocationsButton *locationsButton_;
    ServerRatingIndicator *serverRatingIndicator_;
    MiddleItem *middleItem_;
    FirewallButton *firewallButton_;
    IconButton *firewallInfo_;
    IconButton *preferencesButton_;
    IconButton *networkTrustButton_;

    LogoNotificationsButton *logoButton_;

    LocationID locationID_;
    bool favorite_;

    ProtoTypes::ConnectState prevConnectState_;
    QString networkName_;
    ProtoTypes::NetworkTrustType trustType_;
    ProtoTypes::NetworkInterfaceType interfaceType_;
    bool networkActive_;

    QString fullSecondName_;

    QString connectionTime_;
    QString dataTransferred_;

    bool isFirewallBlocked_;

    void updatePositions();

};

} //namespace ConnectWindow


#endif // CONNECTWINDOWITEM_H
