#include "networkwhitelistwindowitem.h"

#include <QPainter>
#include "languagecontroller.h"
#include "../backend/types/types.h"

namespace PreferencesWindow {

NetworkWhiteListWindowItem::NetworkWhiteListWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage(parent),
    preferences_(preferences)
{
    textItem_ = new TextItem(this, tr(NO_NETWORKS_DETECTED.toStdString().c_str()), 50);
    addItem(textItem_);

    currentNetworkItem_ = new CurrentNetworkItem(this);
    connect(currentNetworkItem_, SIGNAL(currentNetworkTrustChanged(ProtoTypes::NetworkInterface)), SLOT(onCurrentNetworkTrustChanged(ProtoTypes::NetworkInterface)));
    addItem(currentNetworkItem_);

    comboListLabelItem_ = new TextItem(this, tr("OTHER NETWORKS"), 30);
    addItem(comboListLabelItem_);

    networkListItem_ = new NetworkListItem(this);
    addItem(networkListItem_);
    connect(networkListItem_, SIGNAL(networkItemsChanged(ProtoTypes::NetworkInterface)), SLOT(onNetworkListChanged(ProtoTypes::NetworkInterface)));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
    connect(preferences, SIGNAL(networkWhiteListChanged(ProtoTypes::NetworkWhiteList)), SLOT(onNetworkWhiteListPreferencesChanged(ProtoTypes::NetworkWhiteList)));

    addNetworks(preferences_->networkWhiteList());
}

QString NetworkWhiteListWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Network Whitelist");
}

void NetworkWhiteListWindowItem::setCurrentNetwork(ProtoTypes::NetworkInterface networkInterface)
{
    // update CURRENT NETWORK
    if (networkInterface.interface_type() == ProtoTypes::NETWORK_INTERFACE_NONE)
    {
        ProtoTypes::NetworkInterface currentInterface = currentNetworkItem_->currentNetworkInterface();
        networkInterface.set_trust_type(currentInterface.trust_type());
        currentNetworkItem_->setNetworkInterface(networkInterface);
        currentNetworkItem_->setComboVisible(false);
    }
    else
    {
        ProtoTypes::NetworkTrustType trust = trustTypeFromPreferences(networkInterface);
        currentNetworkItem_->setNetworkInterface(networkInterface, trust);
        currentNetworkItem_->setComboVisible(true);
    }

    // update OTHER NETWORKS
    networkListItem_->setCurrentNetwork(networkInterface);
    networkListItem_->updateNetworkCombos();
}

void NetworkWhiteListWindowItem::clearNetworks()
{
    networkListItem_->clearNetworks();
    updateDescription();
}

void NetworkWhiteListWindowItem::initNetworks()
{
    ProtoTypes::NetworkInterface network1;
    network1.set_network_or_ssid("Test Network 1");
    network1.set_active(false);
    network1.set_interface_type(ProtoTypes::NETWORK_INTERFACE_WIFI);

    ProtoTypes::NetworkInterface network2;
    network2.set_network_or_ssid("Test Network 2");
    network2.set_active(false);
    network2.set_interface_type(ProtoTypes::NETWORK_INTERFACE_WIFI);

    ProtoTypes::NetworkInterface network3;
    network3.set_network_or_ssid("Test Network 3");
    network3.set_active(false);
    network3.set_interface_type(ProtoTypes::NETWORK_INTERFACE_WIFI);

    addNetwork( network1 );
    addNetwork( network2 );
    addNetwork( network3 );
}

void NetworkWhiteListWindowItem::addNetwork(ProtoTypes::NetworkInterface network, const ProtoTypes::NetworkTrustType networkTrust)
{
    networkListItem_->addNetwork(network, networkTrust);
    updateDescription();
}

void NetworkWhiteListWindowItem::addNetwork(ProtoTypes::NetworkInterface networkEntry)
{
    addNetwork(networkEntry, networkEntry.trust_type());
}

void NetworkWhiteListWindowItem::addNetworks(ProtoTypes::NetworkWhiteList list)
{
    for (int i = 0; i < list.networks_size(); i++)
    {
        addNetwork(list.networks(i));
    }
}

void NetworkWhiteListWindowItem::updateDescription()
{
    if (networkListItem_->networkWhiteList().networks_size() >= 1)
    {
        textItem_->setText(tr("Windscribe will auto-disconnect when you join an unsecured wifi network"));
    }
    else
    {
        textItem_->setText(tr(NO_NETWORKS_DETECTED.toStdString().c_str()));
    }
}

void NetworkWhiteListWindowItem::addNewlyFoundNetworks()
{
    ProtoTypes::NetworkWhiteList nwl = networkListItem_->networkWhiteList();
    ProtoTypes::NetworkWhiteList newList = preferences_->networkWhiteList();

    QList<ProtoTypes::NetworkInterface> adds;
    for (int i = 0; i < newList.networks_size(); i++)
    {
        bool found = false;
        for (int j = 0; j < nwl.networks_size(); j++)
        {
            if (newList.networks(i).network_or_ssid() == nwl.networks(j).network_or_ssid())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            adds.append(newList.networks(i));
        }
    }

    for (ProtoTypes::NetworkInterface network : qAsConst(adds))
    {
        networkListItem_->addNetwork(network, ProtoTypes::NETWORK_SECURED);
    }
}

void NetworkWhiteListWindowItem::updateScaling()
{
    BasePage::updateScaling();

}

void NetworkWhiteListWindowItem::onNetworkWhiteListPreferencesChanged(ProtoTypes::NetworkWhiteList list)
{
    Q_UNUSED(list);
    addNewlyFoundNetworks();
}

void NetworkWhiteListWindowItem::onLanguageChanged()
{
    comboListLabelItem_->setText(tr("OTHER NETWORKS"));
    networkListItem_->updateNetworkCombos();
    currentNetworkItem_->updateReadableText();
    updateDescription();
}

void NetworkWhiteListWindowItem::onNetworkListChanged(ProtoTypes::NetworkInterface network)
{
    updateDescription();
    preferences_->setNetworkWhiteList(networkListItem_->networkWhiteList());
}

void NetworkWhiteListWindowItem::onCurrentNetworkTrustChanged(ProtoTypes::NetworkInterface network)
{
    // update preferences with new trust state
    ProtoTypes::NetworkWhiteList entries = networkListItem_->networkWhiteList();
    for (int i = 0; i < entries.networks_size(); i++)
    {
        if (entries.networks(i).network_or_ssid() == network.network_or_ssid())
        {
            entries.mutable_networks(i)->set_trust_type(network.trust_type());
            break;
        }
    }
    preferences_->setNetworkWhiteList(entries);
    networkListItem_->setCurrentNetwork(network);
    emit currentNetworkUpdated(network);
}

ProtoTypes::NetworkTrustType NetworkWhiteListWindowItem::trustTypeFromPreferences(ProtoTypes::NetworkInterface network)
{
    ProtoTypes::NetworkTrustType trustType = ProtoTypes::NETWORK_SECURED;

    ProtoTypes::NetworkWhiteList list = preferences_->networkWhiteList();
    for (int i = 0; i < list.networks_size(); i++)
    {
        if (network.network_or_ssid() == list.networks(i).network_or_ssid())
        {
            trustType = list.networks(i).trust_type();
            break;
        }
    }

    return trustType;
}


} // namespace PreferencesWindow
