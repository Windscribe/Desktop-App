#ifndef BOTTOMINFOITEM_H
#define BOTTOMINFOITEM_H

#include <QGraphicsObject>
#include "ibottominfoitem.h"
#include "sharingfeatures/sharingfeatureswindowitem.h"
#include "upgradewidget/upgradewidgetitem.h"

namespace SharingFeatures {

class BottomInfoItem : public ScalableGraphicsObject, public IBottomInfoItem
{
    Q_OBJECT
public:
    explicit BottomInfoItem(QGraphicsObject *parent = nullptr);

    virtual QGraphicsObject * getGraphicsObject() { return this; }
    virtual QRectF boundingRect() const ;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    // upgrade widget functions
    virtual bool isUpgradeWidgetVisible() const ;
    virtual void setDataRemaining(qint64 bytesUsed, qint64 bytesMax);
    virtual void setDaysRemaining(int daysLeft);
    virtual void setExtConfigMode(bool isExtConfigMode);

    // sharing features widget functions
    virtual bool isSharingFeatureVisible() const ;

    virtual void setSecureHotspotFeatures(bool isEnabled, const QString &ssid);
    virtual void setSecureHotspotUsersCount(int usersCount);
    virtual void setProxyGatewayFeatures(bool isEnabled, ProtoTypes::ProxySharingMode mode);
    virtual void setProxyGatewayUsersCount(int usersCount);

    virtual QPixmap getCurrentPixmapShape();

    void setClickable(bool enabled);
    virtual void updateScaling();

signals:
    void heightChanged(int newHeight);
    void upgradeClick();
    void renewClick();
    void loginClick();
    void proxyGatewayClick();
    void secureHotspotClick();

private:
    SharingFeaturesWindowItem *sharingFeaturesWindowItem_;
    UpgradeWidget::UpgradeWidgetItem *upgradeWidgetItem_;

    bool isSecureHotspotEnabled_;
    QString secureHotspotSsid_;

    bool isProxyGatewayEnabled_;
    ProtoTypes::ProxySharingMode proxyGatewayMode_;

    int currentUpgradePosX;
    int currentUpgradePosY;

    int height_;

    void updateDisplay();
    void updateSecureHotspotState();
    void recalcHeight();

    // constants:
    const int WIDTH_ = 332;

    const int UPGRADE_POS_X_DEFAULT = 85;
    const int UPGRADE_WIDTH_DEFAULT = 230;
    const int UPGRADE_POS_Y_DEFAULT = 172;

    const int SHARING_UPGRADE_SPACE = 2;
    const int UPGRADE_POS_X_WIDE = 4;
    const int UPGRADE_WIDTH_EXPANDED = WIDTH_ - 8;

};

}
#endif // BOTTOMINFOITEM_H
