#include "networklistitem.h"

#include "backend/persistentstate.h"
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

void NetworkListItem::addNetwork(types::NetworkInterface network, NETWORK_TRUST_TYPE trustType)
{
    QString networkID = network.networkOrSSid;
    QString friendlyName = network.friendlyName;

    QList<QString> selections = NetworkWhiteListShared::networkTrustTypes();

    ComboBoxItem *combo = new ComboBoxItem(this, friendlyName, QString(), 50, Qt::transparent, 0, true);
    for (int i = 0; i < selections.length(); i++)
    {
        NETWORK_TRUST_TYPE trust = static_cast<NETWORK_TRUST_TYPE>(i);
        combo->addItem(selections[i], trust);
    }

    combo->setCurrentItem(trustType);
    connect(combo, SIGNAL(currentItemChanged(QVariant)), SLOT(onNetworkItemChanged(QVariant)));

    networks_.append(combo);

    recalcHeight();
}

QVector<types::NetworkInterface> NetworkListItem::networkWhiteList()
{
    QVector<types::NetworkInterface> entries;
    for (auto *combo : qAsConst(networks_))
    {
        // Convert friendly name to MAC address ID
        types::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(combo->labelCaption());
        network.trustType = static_cast<NETWORK_TRUST_TYPE>(combo->currentItem().toInt());
        entries << network;
    }
    return entries;
}

void NetworkListItem::updateNetworkCombos()
{
    for (auto *combo : qAsConst(networks_))
    {
        NETWORK_TRUST_TYPE currentTrust = static_cast<NETWORK_TRUST_TYPE>(combo->currentItem().toInt());

        combo->clear();

        QList<QString> selections = NetworkWhiteListShared::networkTrustTypes();

        for (int i = 0; i < selections.length(); i++)
        {
            NETWORK_TRUST_TYPE trust = static_cast<NETWORK_TRUST_TYPE>(i);

            types::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(combo->labelCaption());
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

void NetworkListItem::setCurrentNetwork(types::NetworkInterface network)
{
    currentNetwork_ = network;

    // update current network in the list
    for (auto *combo : qAsConst(networks_))
    {
        if (combo->labelCaption() == currentNetwork_.friendlyName)
        {
            combo->setCurrentItem(network.trustType);
            break;
        }
    }

    recalcHeight();
}

void NetworkListItem::onNetworkItemChanged(QVariant data)
{
    Q_UNUSED(data);

    ComboBoxItem *item = dynamic_cast<ComboBoxItem *>(sender());

    if (item->currentItem() == NETWORK_TRUST_FORGET)
    {
        removeNetworkCombo(item);
    }

    types::NetworkInterface network = NetworkWhiteListShared::networkInterfaceByFriendlyName(item->labelCaption());
    network.trustType =static_cast<NETWORK_TRUST_TYPE>(item->currentItem().toInt());
    emit networkItemsChanged(network);
}

void NetworkListItem::recalcHeight()
{
    int newHeight = 0;
    for (auto *combo : qAsConst(networks_))
    {
        if (combo->labelCaption() != currentNetwork_.friendlyName)
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
