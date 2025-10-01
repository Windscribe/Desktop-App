#include "networklistgroup.h"

#include "backend/persistentstate.h"
#include "networkoptionswindowitem.h"
#include "preferenceswindow/linkitem.h"

namespace PreferencesWindow {

NetworkListGroup::NetworkListGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), shownItems_(0), currentNetwork_(QString())
{
    connect(this, &PreferenceGroup::itemsChanged, this, &NetworkListGroup::updateDisplay);
}

void NetworkListGroup::addNetwork(types::NetworkInterface network, NETWORK_TRUST_TYPE trustType)
{
    LinkItem *item = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK, network.networkOrSsid);
    connect(item, &LinkItem::clicked, this, &NetworkListGroup::onNetworkClicked);
    item->setLinkText(NetworkOptionsWindowItem::trustTypeToString(trustType));
    addItem(item);

    networks_[network.networkOrSsid] = network;
    updateDisplay();
}

void NetworkListGroup::removeNetwork(types::NetworkInterface network)
{
    QList<BaseItem *> list = items();

    networks_.remove(network.networkOrSsid);

    for (BaseItem *i: items())
    {
        LinkItem *item = static_cast<LinkItem *>(i);
        if (network.networkOrSsid == item->title())
        {
            hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
            break;
        }
    }

    if (currentNetwork_ == network.networkOrSsid)
    {
        currentNetwork_ = "";
    }
    updateDisplay();
}

void NetworkListGroup::updateScaling()
{
    BaseItem::updateScaling();
}

types::NetworkInterface NetworkListGroup::currentNetwork()
{
    return networks_[currentNetwork_];
}

void NetworkListGroup::setCurrentNetwork(types::NetworkInterface network)
{
    setCurrentNetwork(network, network.trustType);
}

void NetworkListGroup::setCurrentNetwork(types::NetworkInterface network, NETWORK_TRUST_TYPE type)
{
    bool found = false;

    if (!network.networkOrSsid.isEmpty()) {
        for (auto name : networks_.keys()) {
            if (networks_[name].networkOrSsid == network.networkOrSsid) {
                found = true;
                break;
            }
        }

        if (!found) {
            addNetwork(network, type);
        }
        currentNetwork_ = network.networkOrSsid;
    } else {
        currentNetwork_ = "";
    }
    updateDisplay();
}

void NetworkListGroup::setTrustType(types::NetworkInterface network, NETWORK_TRUST_TYPE type)
{
    for (BaseItem *item: items()) {
        LinkItem *i = static_cast<LinkItem *>(item);

        if (i->title() == network.networkOrSsid) {
            i->setLinkText(NetworkOptionsWindowItem::trustTypeToString(type));
            break;
        }
    }
}

void NetworkListGroup::clear()
{
    PreferenceGroup::clearItems();
    networks_.clear();
    currentNetwork_ = "";

    updateDisplay();
}

void NetworkListGroup::onNetworkClicked()
{
    LinkItem *item = static_cast<LinkItem *>(sender());
    emit networkClicked(networkInterfaceByName(item->title()));
}

void NetworkListGroup::updateNetworks(QVector<types::NetworkInterface> list)
{
    QVector<types::NetworkInterface> toRemove;
    QVector<types::NetworkInterface> toAdd;
    // Check for removed networks

    for (auto name : networks_.keys()) {
        bool found = false;
        for (types::NetworkInterface interface : list) {
            if (interface.networkOrSsid == networks_[name].networkOrSsid) {
                found = true;
                break;
            }
        }

        if (!found) {
            toRemove << networks_[name];
        }
    }

    for (auto network : toRemove) {
        removeNetwork(network);
    }

    // Check for new networks
    for (types::NetworkInterface interface : list) {
        bool found = false;
        if (interface.networkOrSsid.isEmpty()) {
            continue;
        }
        for (auto name : networks_.keys()) {
            if (interface.networkOrSsid == networks_[name].networkOrSsid) {
                setTrustType(networks_[name], interface.trustType);
                found = true;
                break;
            }
        }

        if (!found) {
            toAdd << interface;
        }
    }

    for (auto network : toAdd) {
        addNetwork(network, network.trustType);
    }
}

int NetworkListGroup::isEmpty()
{
    return shownItems_ == 0;
}

void NetworkListGroup::updateDisplay()
{
    QList<BaseItem *> list = items();
    int shownItems = 0;

    for (BaseItem *i: items())
    {
        LinkItem *item = static_cast<LinkItem *>(i);
        // Don't show current network here
        if (item->title() == currentNetwork_) {
            hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
        } else {
            showItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
            shownItems++;
        }
    }

    if ((shownItems == 0 && shownItems_ != 0) || (shownItems != 0 && shownItems_ == 0)) {
        shownItems_ = shownItems;
        emit isEmptyChanged();
    }
    shownItems_ = shownItems;
}

types::NetworkInterface NetworkListGroup::networkInterfaceByName(QString networkOrSsid)
{
    QVector<types::NetworkInterface> list = PersistentState::instance().networkWhitelist();
    for (types::NetworkInterface interface : list)
    {
        if (interface.networkOrSsid == networkOrSsid)
        {
            return interface;
        }
    }
    return {};
}

}
