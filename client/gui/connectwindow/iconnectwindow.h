#ifndef ICONNECTWINDOW_H
#define ICONNECTWINDOW_H

#include <QString>
#include <QGraphicsObject>
#include "types/pingtime.h"
#include "types/locationid.h"
#include "types/connectstate.h"
#include "../backend/preferences/preferences.h"
#include "tooltips/tooltiptypes.h"

// abstract interface for login window
class IConnectWindow
{
public:
    virtual ~IConnectWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setClickable(bool isClickable) = 0;

    virtual QRegion getMask() = 0;
    virtual QPixmap getShadowPixmap() = 0;

    virtual void setConnectionTimeAndData(QString connectionTime, QString dataTransferred) = 0;
    virtual void setFirewallAlwaysOn(bool isFirewallAlwaysOn) = 0;
    virtual void setFirewallBlock(bool isFirewallBlocked) = 0;
    virtual void setTestTunnelResult(bool success) = 0;
    virtual void updateScaling() = 0;
    virtual void setProtocolPort(const PROTOCOL &protocol, const uint port) = 0;

public slots:
    virtual void updateLocationInfo(const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime) = 0;
    virtual void updateConnectState(const types::ConnectState & newConnectState) = 0;
    virtual void updateFirewallState(bool isFirewallEnabled) = 0;
    virtual void updateLocationsState(bool isExpanded) = 0;
    virtual void updateMyIp(const QString &ip) = 0;
    virtual void updateNotificationsState(int totalMessages, int unread) = 0;
    virtual void updateNetworkState(types::NetworkInterface network) = 0;
    virtual void setSplitTunnelingState(bool on) = 0;
    virtual void setInternetConnectivity(bool connectivity) = 0;

signals:
    virtual void minimizeClick() = 0;
    virtual void closeClick() = 0;
    virtual void preferencesClick() = 0;
    virtual void connectClick() = 0;
    virtual void firewallClick() = 0;
    virtual void locationsClick() = 0;
    virtual void notificationsClick() = 0;

};

Q_DECLARE_INTERFACE(IConnectWindow, "IConnectWindow")

#endif // ICONNECTWINDOW_H
