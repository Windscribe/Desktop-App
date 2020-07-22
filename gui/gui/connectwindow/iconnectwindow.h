#ifndef ICONNECTWINDOW_H
#define ICONNECTWINDOW_H

#include <QString>
#include <QGraphicsObject>
#include "../Backend/Types/pingtime.h"
#include "../Backend/Types/types.h"
#include "../Backend/Types/locationid.h"
#include "../Backend/Preferences/preferences.h"
#include "IPC/generated_proto/types.pb.h"
#include "Tooltips/tooltiptypes.h"

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
    virtual void setProtocolPort(const ProtoTypes::Protocol &protocol, const uint port) = 0;

public slots:
    virtual void updateLocationInfo(LocationID id, const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isFavorite) = 0;
    virtual void updateLocationSpeed(LocationID id, PingTime speed) = 0;
    virtual void updateConnectState(const ProtoTypes::ConnectState & newConnectState) = 0;
    virtual void updateFirewallState(bool isFirewallEnabled) = 0;
    virtual void updateLocationsState(bool isExpanded) = 0;
    virtual void updateFavoriteState(LocationID id, bool isFavorite) = 0;
    virtual void updateMyIp(const QString &ip) = 0;
    virtual void updateNotificationsState(int totalMessages, int unread) = 0;
    virtual void updateNetworkState(ProtoTypes::NetworkInterface network) = 0;
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
    virtual void showTooltip(TooltipInfo info) = 0;
    virtual void hideTooltip(TooltipId type) = 0;

};

Q_DECLARE_INTERFACE(IConnectWindow, "IConnectWindow")

#endif // ICONNECTWINDOW_H
