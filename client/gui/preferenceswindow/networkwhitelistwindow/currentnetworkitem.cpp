#include "currentnetworkitem.h"

#include <QPainter>
#include "../dividerline.h"
#include "graphicresources/fontmanager.h"
#include "networkwhitelistshared.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

CurrentNetworkItem::CurrentNetworkItem(ScalableGraphicsObject *parent) : BaseItem(parent, 78)
{
    // init current network combo
    QList<QString> selections = NetworkWhiteListShared::networkTrustTypesWithoutForget();
    currentNetworkCombo_ = new ComboBoxItem(this, tr(""), QString(), 50, Qt::transparent, 0, true);
    currentNetworkCombo_->setVisible(false);
    for (int i = 0; i < selections.length(); i++)
    {
        NETWORK_TRUST_TYPE trust = static_cast<NETWORK_TRUST_TYPE>(i);
        currentNetworkCombo_->addItem(selections[i], trust);
    }
    currentNetworkCombo_->setCurrentItem(NETWORK_TRUST_SECURED);
    connect(currentNetworkCombo_, SIGNAL(currentItemChanged(QVariant)), SLOT(onTrustTypeChanged(QVariant)));
    updatePositions();
}

void CurrentNetworkItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // CURRENT NETWORK text
    qreal initialOpacity = painter->opacity();
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(0.5 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 18*G_SCALE, 0, 0), Qt::AlignTop, tr("CURRENT NETWORK"));

    if (!currentNetworkCombo_->isVisible())
    {
        painter->setOpacity(1.0 * initialOpacity);
        painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, -16*G_SCALE), Qt::AlignBottom, tr("No Network Info"));

        painter->setOpacity(0.15 * initialOpacity);
        painter->setPen(Qt::white);

        const int offsetPosY = 78*G_SCALE;
        painter->drawLine(24*G_SCALE, offsetPosY + 0, 280*G_SCALE, offsetPosY + 0);
        painter->drawLine(24*G_SCALE, offsetPosY + 1, 280*G_SCALE, offsetPosY + 1);
    }
}

void CurrentNetworkItem::setComboVisible(bool visible)
{
    currentNetworkCombo_->setVisible(visible);
}

void CurrentNetworkItem::setNetworkInterface(types::NetworkInterface network)
{
    setNetworkInterface(network, network.trustType);
}

void CurrentNetworkItem::setNetworkInterface(types::NetworkInterface network, NETWORK_TRUST_TYPE newTrustType)
{
    networkInterface_ = network;
    networkInterface_.trustType = newTrustType;

    QString caption = network.friendlyName;
    if (caption == "") caption = tr("No Network Info");

    currentNetworkCombo_->setLabelCaption(caption);
    currentNetworkCombo_->setCurrentItem(newTrustType);
    update();
}

types::NetworkInterface CurrentNetworkItem::currentNetworkInterface()
{
    return networkInterface_;
}

void CurrentNetworkItem::hideOpenPopups()
{
    currentNetworkCombo_->hideOpenPopups();
}

void CurrentNetworkItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(78*G_SCALE);
    updatePositions();
}

void CurrentNetworkItem::updateReadableText()
{
    // update caption
    QString caption = networkInterface_.friendlyName;
    if (caption == "") caption = tr("No Network Info");
    currentNetworkCombo_->setLabelCaption(caption);

    // update combo text
    NETWORK_TRUST_TYPE currentTrust = static_cast<NETWORK_TRUST_TYPE>(currentNetworkCombo_->currentItem().toInt());
    currentNetworkCombo_->clear();
    QList<QString> selections = NetworkWhiteListShared::networkTrustTypes();
    for (int i = 0; i < selections.length(); i++)
    {
        NETWORK_TRUST_TYPE trust = static_cast<NETWORK_TRUST_TYPE>(i);

        types::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(currentNetworkCombo_->labelCaption());
        currentNetworkCombo_->addItem(selections[i], trust);
    }
    currentNetworkCombo_->setCurrentItem(currentTrust);
}

void CurrentNetworkItem::onTrustTypeChanged(const QVariant & /*value*/)
{
    types::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(currentNetworkCombo_->labelCaption());
    network.trustType = static_cast<NETWORK_TRUST_TYPE>(currentNetworkCombo_->currentItem().toInt());
    emit currentNetworkTrustChanged(network);
}

void CurrentNetworkItem::updatePositions()
{
    currentNetworkCombo_->setPos(0, 30*G_SCALE);
}

}
