#pragma once

#include <QGraphicsObject>
#include "sharingfeatures/sharingfeatureswindowitem.h"
#include "upgradewidget/upgradewidgetitem.h"

namespace SharingFeatures {

class BottomInfoItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit BottomInfoItem(Preferences *preferences, QGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    // upgrade widget functions
    bool isUpgradeWidgetVisible() const;
    void setDataRemaining(qint64 bytesUsed, qint64 bytesMax);
    void setDaysRemaining(int daysLeft);
    void setExtConfigMode(bool isExtConfigMode);

    // sharing features widget functions
    bool isSharingFeatureVisible() const;

    void setSecureHotspotFeatures(bool isEnabled, const QString &ssid);
    void setSecureHotspotUsersCount(int usersCount);
    void setProxyGatewayFeatures(bool isEnabled, PROXY_SHARING_TYPE mode);
    void setProxyGatewayUsersCount(int usersCount);

    QPixmap getCurrentPixmapShape();

    void setClickable(bool enabled);

private slots:
    void updateDisplay();

signals:
    void heightChanged(int newHeight);
    void upgradeClick();
    void renewClick();
    void loginClick();
    void proxyGatewayClick();
    void secureHotspotClick();

private:
    Preferences *preferences_;

    SharingFeaturesWindowItem *sharingFeaturesWindowItem_;
    UpgradeWidget::UpgradeWidgetItem *upgradeWidgetItem_;

    bool isSecureHotspotEnabled_;
    QString secureHotspotSsid_;

    bool isProxyGatewayEnabled_;
    PROXY_SHARING_TYPE proxyGatewayMode_;

    int currentUpgradePosX;
    int currentUpgradePosY;

    int height_;

    void updateSecureHotspotState();
    void recalcHeight();

    // constants:
    static constexpr int WIDTH_ = 350;

    static constexpr int UPGRADE_POS_X_DEFAULT = 85;
    static constexpr int UPGRADE_WIDTH_DEFAULT = 248;
    static constexpr int UPGRADE_POS_Y_DEFAULT = 172;

    static constexpr int SHARING_UPGRADE_SPACE = 2;
    static constexpr int UPGRADE_POS_X_WIDE = 4;
    static constexpr int UPGRADE_WIDTH_EXPANDED = WIDTH_ - 8;
    static constexpr int UPGRADE_WIDTH_VAN_GOGH = WIDTH_;
};

}
