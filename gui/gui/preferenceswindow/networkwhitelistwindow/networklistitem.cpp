#include "networklistitem.h"

#include "../backend/types/types.h"
#include "../backend/persistentstate.h"
#include "networkwhitelistshared.h"

namespace PreferencesWindow {

NetworkListItem::NetworkListItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
{

}

void NetworkListItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void NetworkListItem::hideOpenPopups()
{
    for (auto *combo : qAsConst(networks_))
    {
        combo->hideOpenPopups();
    }
}

void NetworkListItem::clearNetworks()
{
    for (auto *combo : qAsConst(networks_))
    {
        removeNetworkCombo(combo);
    }
}

void NetworkListItem::addNetwork(ProtoTypes::NetworkInterface network, ProtoTypes::NetworkTrustType trustType)
{
    QString networkID = QString::fromStdString(network.network_or_ssid());
    QString friendlyName = QString::fromStdString(network.friendly_name());

    QList<QString> selections = NetworkWhiteListShared::networkTrustTypes();

    ComboBoxItem *combo = new ComboBoxItem(this, friendlyName, QString(), 50, Qt::transparent, 0, true);
    for (int i = 0; i < selections.length(); i++)
    {
        ProtoTypes::NetworkTrustType trust = static_cast<ProtoTypes::NetworkTrustType>(i);
        combo->addItem(selections[i], trust);
    }

    combo->setCurrentItem(trustType);
    connect(combo, SIGNAL(currentItemChanged(QVariant)), SLOT(onNetworkItemChanged(QVariant)));

    networks_.append(combo);

    recalcHeight();
}

ProtoTypes::NetworkWhiteList NetworkListItem::networkWhiteList()
{
    ProtoTypes::NetworkWhiteList entries;
    for (auto *combo : qAsConst(networks_))
    {
        // Convert friendly name to MAC address ID
        ProtoTypes::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(combo->labelCaption());
        network.set_trust_type(static_cast<ProtoTypes::NetworkTrustType>(combo->currentItem().toInt()));
        *entries.add_networks() = network;
    }
    return entries;
}

void NetworkListItem::updateNetworkCombos()
{
    for (auto *combo : qAsConst(networks_))
    {
        ProtoTypes::NetworkTrustType currentTrust = static_cast<ProtoTypes::NetworkTrustType>(combo->currentItem().toInt());

        combo->clear();

        QList<QString> selections = NetworkWhiteListShared::networkTrustTypes();

        for (int i = 0; i < selections.length(); i++)
        {
            ProtoTypes::NetworkTrustType trust = static_cast<ProtoTypes::NetworkTrustType>(i);

            ProtoTypes::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(combo->labelCaption());
            combo->addItem(selections[i], trust);
        }
        combo->setCurrentItem(currentTrust);
    }
}

void NetworkListItem::updateScaling()
{
    BaseItem::updateScaling();
    recalcHeight();
}

void NetworkListItem::setCurrentNetwork(ProtoTypes::NetworkInterface network)
{
    currentNetwork_ = network;

    // update current network in the list
    for (auto *combo : qAsConst(networks_))
    {
        if (combo->labelCaption() == QString::fromStdString(currentNetwork_.friendly_name()))
        {
            combo->setCurrentItem(network.trust_type());
            break;
        }
    }

    recalcHeight();
}

void NetworkListItem::onNetworkItemChanged(QVariant data)
{
    ComboBoxItem *item = dynamic_cast<ComboBoxItem *>(sender());

    if (item->currentItem() == ProtoTypes::NETWORK_FORGET)
    {
        removeNetworkCombo(item);
    }

    ProtoTypes::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(item->labelCaption());
    network.set_trust_type(static_cast<ProtoTypes::NetworkTrustType>(item->currentItem().toInt()));
    emit networkItemsChanged(network);
}

void NetworkListItem::recalcHeight()
{
    int newHeight = 0;
    for (auto *combo : qAsConst(networks_))
    {
        if (combo->labelCaption() != QString::fromStdString(currentNetwork_.friendly_name()))
        {
            combo->setPos(0, newHeight);
            newHeight += combo->boundingRect().height();
            combo->show();
        }
        else
        {
            combo->hide();
        }
    }

    setHeight(newHeight);
}

void NetworkListItem::removeNetworkCombo(ComboBoxItem *combo)
{
    combo->disconnect();
    networks_.removeOne(combo);
    combo->deleteLater();

    recalcHeight();
}

}
