#include "bottominfoitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"

namespace SharingFeatures {

BottomInfoItem::BottomInfoItem(Preferences *preferences, QGraphicsObject *parent) : ScalableGraphicsObject(parent),
    preferences_(preferences),
    sharingFeaturesWindowItem_(NULL),
    upgradeWidgetItem_(NULL),
    isSecureHotspotEnabled_(false),
    isProxyGatewayEnabled_(false),
    proxyGatewayMode_(PROXY_SHARING_HTTP)
{
    setFlag(ItemHasNoContents, false);

    height_ = 0;

    currentUpgradePosX = 0;
    currentUpgradePosY = 0;

    connect(preferences_, &Preferences::appSkinChanged, this, &BottomInfoItem::updateDisplay);
}

QRectF BottomInfoItem::boundingRect() const
{
    return QRectF(0, 0, WIDTH_ * G_SCALE, height_);
}

void BottomInfoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // needed so class not abstract
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

bool BottomInfoItem::isUpgradeWidgetVisible() const
{
    return upgradeWidgetItem_ != NULL;
}

void BottomInfoItem::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    if (bytesUsed == -1 || bytesMax == -1)
    {
        SAFE_DELETE(upgradeWidgetItem_);
    }
    else
    {
        if (upgradeWidgetItem_ == NULL)
        {
            upgradeWidgetItem_ = new UpgradeWidget::UpgradeWidgetItem(this);
            connect(upgradeWidgetItem_, &UpgradeWidget::UpgradeWidgetItem::buttonClick, this, &BottomInfoItem::upgradeClick);
            upgradeWidgetItem_->setDataRemaining(bytesUsed, bytesMax);
        }
        else
        {
            upgradeWidgetItem_->setDataRemaining(bytesUsed, bytesMax);
        }
    }

    updateDisplay();
}

void BottomInfoItem::setDaysRemaining(int daysLeft)
{
    if (daysLeft == -1)
    {
        SAFE_DELETE(upgradeWidgetItem_);
    }
    else
    {
        if (upgradeWidgetItem_ == NULL)
        {
            upgradeWidgetItem_ = new UpgradeWidget::UpgradeWidgetItem(this);
            connect(upgradeWidgetItem_, &UpgradeWidget::UpgradeWidgetItem::buttonClick, this, &BottomInfoItem::renewClick);
        }
        upgradeWidgetItem_->setDaysRemaining(daysLeft);
    }
    updateDisplay();
}

void BottomInfoItem::setExtConfigMode(bool isExtConfigMode)
{
    if (!isExtConfigMode)
    {
        SAFE_DELETE(upgradeWidgetItem_);
    }
    else
    {
        if (upgradeWidgetItem_ == NULL)
        {
            upgradeWidgetItem_ = new UpgradeWidget::UpgradeWidgetItem(this);
            connect(upgradeWidgetItem_, &UpgradeWidget::UpgradeWidgetItem::buttonClick, this, &BottomInfoItem::loginClick);
        }
        upgradeWidgetItem_->setExtConfigMode(isExtConfigMode);
    }
    updateDisplay();
}

bool BottomInfoItem::isSharingFeatureVisible() const
{
    return isSecureHotspotEnabled_ || isProxyGatewayEnabled_;
}

void BottomInfoItem::setSecureHotspotFeatures(bool isEnabled, const QString &ssid)
{
    isSecureHotspotEnabled_ = isEnabled;
    secureHotspotSsid_ = ssid;
    updateSecureHotspotState();
}

void BottomInfoItem::setSecureHotspotUsersCount(int usersCount)
{
    if (sharingFeaturesWindowItem_ != NULL)
    {
        sharingFeaturesWindowItem_->setSecureHotspotUsersCount(usersCount);
    }
}

void BottomInfoItem::setProxyGatewayFeatures(bool isEnabled, PROXY_SHARING_TYPE mode)
{
    isProxyGatewayEnabled_ = isEnabled;
    proxyGatewayMode_ = mode;
    updateSecureHotspotState();
}

void BottomInfoItem::setProxyGatewayUsersCount(int usersCount)
{
    if (sharingFeaturesWindowItem_ != NULL)
    {
        sharingFeaturesWindowItem_->setProxyGatewayUsersCount(usersCount);
    }
}

QPixmap BottomInfoItem::getCurrentPixmapShape()
{
    QPixmap tempPixmap(WIDTH_* G_SCALE, height_);
    tempPixmap.fill(Qt::transparent);

    QPainter painter(&tempPixmap);

    if (upgradeWidgetItem_ != NULL)
    {
        painter.drawPixmap(currentUpgradePosX, currentUpgradePosY, upgradeWidgetItem_->getCurrentPixmapShape());
    }

    if (sharingFeaturesWindowItem_ != NULL)
    {
        painter.drawPixmap(0, 0, sharingFeaturesWindowItem_->getCurrentPixmapShape());
    }

    return tempPixmap;
}

void BottomInfoItem::updateDisplay()
{
    if (upgradeWidgetItem_ != NULL)
    {
        if (isSharingFeatureVisible())
        {
            sharingFeaturesWindowItem_->setHorns(true);

            int upgradePosY = sharingFeaturesWindowItem_->currentHeight() + SHARING_UPGRADE_SPACE*G_SCALE;

            currentUpgradePosX = UPGRADE_POS_X_WIDE*G_SCALE;
            currentUpgradePosY = upgradePosY;

            upgradeWidgetItem_->setPos(currentUpgradePosX, currentUpgradePosY);
            upgradeWidgetItem_->setWidth(UPGRADE_WIDTH_EXPANDED);
        }
        else
        {
            if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
            {
                upgradeWidgetItem_->setPos(0, 0);
                upgradeWidgetItem_->setWidth(UPGRADE_WIDTH_VAN_GOGH);
            }
            else
            {
                currentUpgradePosX = 0;
                currentUpgradePosY = 0;
                upgradeWidgetItem_->setPos(0, 0);
                upgradeWidgetItem_->setWidth(UPGRADE_WIDTH_DEFAULT);
            }
        }
    }
    else
    {
        if (sharingFeaturesWindowItem_ != NULL)
        {
            sharingFeaturesWindowItem_->setHorns(false);
        }
    }

    recalcHeight();
}

void BottomInfoItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updateDisplay();
}

void BottomInfoItem::setClickable(bool enabled)
{
    if (sharingFeaturesWindowItem_ != nullptr)
    {
        sharingFeaturesWindowItem_->setClickable(enabled);
    }
    if (upgradeWidgetItem_ != nullptr)
    {
        upgradeWidgetItem_->setClickable(enabled);
    }
}

void BottomInfoItem::updateSecureHotspotState()
{
    if (!isSharingFeatureVisible())
    {
        SAFE_DELETE(sharingFeaturesWindowItem_);
        updateDisplay();
        return;
    }
    else if (isSharingFeatureVisible() && sharingFeaturesWindowItem_ == NULL)
    {
        sharingFeaturesWindowItem_ = new SharingFeaturesWindowItem(preferences_, this);
        connect(sharingFeaturesWindowItem_, &SharingFeaturesWindowItem::clickedProxy, this, &BottomInfoItem::proxyGatewayClick);
        connect(sharingFeaturesWindowItem_, &SharingFeaturesWindowItem::clickedHotSpot, this, &BottomInfoItem::secureHotspotClick);
    }

    sharingFeaturesWindowItem_->setSecureHotspotFeatures(isSecureHotspotEnabled_, secureHotspotSsid_);
    sharingFeaturesWindowItem_->setProxyGatewayFeatures(isProxyGatewayEnabled_, proxyGatewayMode_);
    updateDisplay();
}

void BottomInfoItem::recalcHeight()
{
    int newHeight = 0;
    if (sharingFeaturesWindowItem_ != NULL) newHeight += sharingFeaturesWindowItem_->currentHeight();
    if (upgradeWidgetItem_ != NULL) newHeight += upgradeWidgetItem_->getGraphicsObject()->boundingRect().height() + SHARING_UPGRADE_SPACE*G_SCALE;

    if ( newHeight != height_)
    {
        prepareGeometryChange();
        height_ = newHeight;
        emit heightChanged(newHeight);
    }
}

}
