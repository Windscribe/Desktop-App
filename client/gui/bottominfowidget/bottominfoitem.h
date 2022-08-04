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

    QGraphicsObject *getGraphicsObject() override { return this; }
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    // upgrade widget functions
    bool isUpgradeWidgetVisible() const override;
    void setDataRemaining(qint64 bytesUsed, qint64 bytesMax) override;
    void setDaysRemaining(int daysLeft) override;
    void setExtConfigMode(bool isExtConfigMode) override;

    // sharing features widget functions
    bool isSharingFeatureVisible() const override;

    void setSecureHotspotFeatures(bool isEnabled, const QString &ssid) override;
    void setSecureHotspotUsersCount(int usersCount) override;
    void setProxyGatewayFeatures(bool isEnabled, PROXY_SHARING_TYPE mode) override;
    void setProxyGatewayUsersCount(int usersCount) override;

    QPixmap getCurrentPixmapShape() override;

    void setClickable(bool enabled) override;
    void updateScaling() override;

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
    PROXY_SHARING_TYPE proxyGatewayMode_;

    int currentUpgradePosX;
    int currentUpgradePosY;

    int height_;

    void updateDisplay();
    void updateSecureHotspotState();
    void recalcHeight();

    // constants:
    static constexpr int WIDTH_ = 332;

    static constexpr int UPGRADE_POS_X_DEFAULT = 85;
    static constexpr int UPGRADE_WIDTH_DEFAULT = 230;
    static constexpr int UPGRADE_POS_Y_DEFAULT = 172;

    static constexpr int SHARING_UPGRADE_SPACE = 2;
    static constexpr int UPGRADE_POS_X_WIDE = 4;
    static constexpr int UPGRADE_WIDTH_EXPANDED = WIDTH_ - 8;

};

}
#endif // BOTTOMINFOITEM_H
