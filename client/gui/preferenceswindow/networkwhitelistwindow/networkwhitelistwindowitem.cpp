#include "networkwhitelistwindowitem.h"

#include <QPainter>
#include "languagecontroller.h"

namespace PreferencesWindow {

NetworkWhiteListWindowItem::NetworkWhiteListWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage(parent),
    preferences_(preferences)
{
    textItem_ = new TextItem(this, tr(NO_NETWORKS_DETECTED.toStdString().c_str()), 50);
    addItem(textItem_);

    currentNetworkItem_ = new CurrentNetworkItem(this);
    connect(currentNetworkItem_, SIGNAL(currentNetworkTrustChanged(types::NetworkInterface)), SLOT(onCurrentNetworkTrustChanged(types::NetworkInterface)));
    addItem(currentNetworkItem_);

    comboListLabelItem_ = new TextItem(this, tr("OTHER NETWORKS"), 30);
    addItem(comboListLabelItem_);

    networkListItem_ = new NetworkListItem(this);
    addItem(networkListItem_);
    connect(networkListItem_, SIGNAL(networkItemsChanged(types::NetworkInterface)), SLOT(onNetworkListChanged(types::NetworkInterface)));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
    connect(preferences, SIGNAL(networkWhiteListChanged(QVector<types::NetworkInterface>)), SLOT(onNetworkWhiteListPreferencesChanged(QVector<types::NetworkInterface>)));

    addNetworks(preferences_->networkWhiteList());
}

QString NetworkWhiteListWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Network Whitelist");
}

void NetworkWhiteListWindowItem::setCurrentNetwork(types::NetworkInterface networkInterface)
{
    // update CURRENT NETWORK
    if (networkInterface.interfaceType == NETWORK_INTERFACE_NONE)
    {
        types::NetworkInterface currentInterface = currentNetworkItem_->currentNetworkInterface();
        networkInterface.trustType = currentInterface.trustType;
        currentNetworkItem_->setNetworkInterface(networkInterface);
        currentNetworkItem_->setComboVisible(false);
    }
    else
    {
        NETWORK_TRUST_TYPE trust = trustTypeFromPreferences(networkInterface);
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
    types::NetworkInterface network1;
    network1.networkOrSSid = "Test Network 1";
    network1.active = false;
    network1.interfaceType = NETWORK_INTERFACE_WIFI;

    types::NetworkInterface network2;
    network2.networkOrSSid = "Test Network 2";
    network2.active = false;
    network2.interfaceType = NETWORK_INTERFACE_WIFI;

    types::NetworkInterface network3;
    network3.networkOrSSid = "Test Network 3";
    network3.active = false;
    network3.interfaceType = NETWORK_INTERFACE_WIFI;

    addNetwork( network1 );
    addNetwork( network2 );
    addNetwork( network3 );
}

void NetworkWhiteListWindowItem::addNetwork(types::NetworkInterface network, const NETWORK_TRUST_TYPE networkTrust)
{
    networkListItem_->addNetwork(network, networkTrust);
    updateDescription();
}

void NetworkWhiteListWindowItem::addNetwork(types::NetworkInterface networkEntry)
{
    addNetwork(networkEntry, networkEntry.trustType);
}

void NetworkWhiteListWindowItem::addNetworks(QVector<types::NetworkInterface> list)
{
    for (int i = 0; i < list.size(); i++)
    {
        addNetwork(list[i]);
    }
}

void NetworkWhiteListWindowItem::updateDescription()
{
    if (networkListItem_->networkWhiteList().size() >= 1)
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
    QVector<types::NetworkInterface> nwl = networkListItem_->networkWhiteList();
    QVector<types::NetworkInterface> newList = preferences_->networkWhiteList();

    QList<types::NetworkInterface> adds;
    for (int i = 0; i < newList.size(); i++)
    {
        bool found = false;
        for (int j = 0; j < nwl.size(); j++)
        {
            if (newList[i].networkOrSSid == nwl[j].networkOrSSid)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            adds.append(newList[i]);
        }
    }

    for (types::NetworkInterface network : qAsConst(adds))
    {
        networkListItem_->addNetwork(network, NETWORK_TRUST_SECURED);
    }
}

void NetworkWhiteListWindowItem::updateScaling()
{
    BasePage::updateScaling();

}

void NetworkWhiteListWindowItem::onNetworkWhiteListPreferencesChanged(QVector<types::NetworkInterface> list)
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

void NetworkWhiteListWindowItem::onNetworkListChanged(types::NetworkInterface network)
{
    Q_UNUSED(network);
    updateDescription();
    preferences_->setNetworkWhiteList(networkListItem_->networkWhiteList());
}

void NetworkWhiteListWindowItem::onCurrentNetworkTrustChanged(types::NetworkInterface network)
{
    // update preferences with new trust state
    QVector<types::NetworkInterface> entries = networkListItem_->networkWhiteList();
    for (int i = 0; i < entries.size(); i++)
    {
        if (entries[i].networkOrSSid == network.networkOrSSid)
        {
            entries[i].trustType = network.trustType;
            break;
        }
    }
    preferences_->setNetworkWhiteList(entries);
    networkListItem_->setCurrentNetwork(network);
    emit currentNetworkUpdated(network);
}

NETWORK_TRUST_TYPE NetworkWhiteListWindowItem::trustTypeFromPreferences(types::NetworkInterface network)
{
    NETWORK_TRUST_TYPE trustType = NETWORK_TRUST_SECURED;

    QVector<types::NetworkInterface> list = preferences_->networkWhiteList();
    for (int i = 0; i < list.size(); i++)
    {
        if (network.networkOrSSid == list[i].networkOrSSid)
        {
            trustType = list[i].trustType;
            break;
        }
    }

    return trustType;
}


} // namespace PreferencesWindow
