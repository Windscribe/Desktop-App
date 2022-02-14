#ifndef IBOTTOMINFOITEM_H
#define IBOTTOMINFOITEM_H

#include <QGraphicsObject>
#include "../backend/preferences/preferences.h"

class IBottomInfoItem
{
public:
    virtual ~IBottomInfoItem() {}

    virtual QGraphicsObject * getGraphicsObject() = 0;

    // upgrade widget functions
    virtual bool isUpgradeWidgetVisible() const = 0;
    virtual void setDataRemaining(qint64 bytesUsed, qint64 bytesMax) = 0; // gbLeft from 0 to gbMax; if gbLeft == -1 or gbMax == -1 -> hide widget
    virtual void setDaysRemaining(int daysLeft) = 0;
    virtual void setExtConfigMode(bool isExtConfigMode) = 0;

    // sharing features widget functions
    virtual bool isSharingFeatureVisible() const = 0;

    virtual void setSecureHotspotFeatures(bool isEnabled, const QString &ssid) = 0;
    virtual void setSecureHotspotUsersCount(int usersCount) = 0;
    virtual void setProxyGatewayFeatures(bool isEnabled, ProtoTypes::ProxySharingMode mode) = 0;
    virtual void setProxyGatewayUsersCount(int usersCount) = 0;

    virtual QPixmap getCurrentPixmapShape() = 0;

    virtual void setClickable(bool enabled) = 0;

    virtual void updateScaling() = 0;

signals:
    void heightChanged(int newHeight);
    void upgradeClick();
    void loginClick();
    void proxyGatewayClick();
    void secureHotspotClick();
};


#endif // IBOTTOMINFOITEM_H
