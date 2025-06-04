#include "networkoptionswindowitem.h"

#include <QPainter>
#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

NetworkOptionsWindowItem::NetworkOptionsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences), currentScreen_(NETWORK_OPTIONS_HOME)
{
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    desc_ = new PreferenceGroup(this);
    addItem(desc_);

    autosecureGroup_ = new PreferenceGroup(this);
    autosecureCheckbox_ = new ToggleItem(autosecureGroup_);
    autosecureCheckbox_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/AUTOSECURE_NETWORKS"));
    autosecureCheckbox_->setState(preferences->isAutoSecureNetworks());
    connect(autosecureCheckbox_, &ToggleItem::stateChanged, this, &NetworkOptionsWindowItem::onAutoSecureChanged);
    autosecureGroup_->addItem(autosecureCheckbox_);
    addItem(autosecureGroup_);

    currentNetworkTitle_ = new TitleItem(this);
    addItem(currentNetworkTitle_);

    currentNetworkGroup_ = new PreferenceGroup(this);
    placeholderItem_ = new LinkItem(currentNetworkGroup_, LinkItem::LinkType::TEXT_ONLY);
    currentNetworkItem_ = placeholderItem_;
    currentNetworkGroup_->addItem(placeholderItem_);
    addItem(currentNetworkGroup_);

    otherNetworksTitle_ = new TitleItem(this);
    addItem(otherNetworksTitle_);

    otherNetworksGroup_ = new NetworkListGroup(this);
    addItem(otherNetworksGroup_);

    connect(otherNetworksGroup_, &NetworkListGroup::networkClicked, this, &NetworkOptionsWindowItem::networkClicked);
    connect(otherNetworksGroup_, &NetworkListGroup::isEmptyChanged, this, &NetworkOptionsWindowItem::updateShownItems);
    connect(preferences, &Preferences::networkWhiteListChanged, this, &NetworkOptionsWindowItem::onNetworkWhitelistChanged);

    addNetworks(preferences_->networkWhiteList());

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &NetworkOptionsWindowItem::onLanguageChanged);
    onLanguageChanged();

    updateShownItems();
}

QString NetworkOptionsWindowItem::caption() const
{
    return tr("Network Options");
}

void NetworkOptionsWindowItem::setCurrentNetwork(types::NetworkInterface network)
{
    currentNetwork_ = network;
    currentNetworkGroup_->clearItems(true);

    if (!network.networkOrSsid.isEmpty()) {
        currentNetworkItem_ = new LinkItem(currentNetworkGroup_, LinkItem::LinkType::SUBPAGE_LINK, network.networkOrSsid);
        connect(currentNetworkItem_, &LinkItem::clicked, this, &NetworkOptionsWindowItem::onCurrentNetworkClicked);
        currentNetworkGroup_->addItem(currentNetworkItem_);
        currentNetworkGroup_->hideItems(indexOf(placeholderItem_), -1, PreferenceGroup::DISPLAY_FLAGS::FLAG_NO_ANIMATION);
        currentNetworkItem_->setLinkText(trustTypeToString(network.trustType));

        otherNetworksGroup_->setCurrentNetwork(network);
    } else {
        currentNetworkItem_ = placeholderItem_;
        currentNetworkGroup_->showItems(indexOf(placeholderItem_), -1, PreferenceGroup::DISPLAY_FLAGS::FLAG_NO_ANIMATION);
        otherNetworksGroup_->setCurrentNetwork(network);
    }
    updateShownItems();
}

void NetworkOptionsWindowItem::clearNetworks()
{
    currentNetworkGroup_->clearItems(true);
    currentNetworkItem_ = placeholderItem_;
    currentNetworkGroup_->showItems(indexOf(placeholderItem_), -1, PreferenceGroup::DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    otherNetworksGroup_->clear();
}

void NetworkOptionsWindowItem::addNetwork(types::NetworkInterface network, const NETWORK_TRUST_TYPE networkTrust)
{
    otherNetworksGroup_->addNetwork(network, networkTrust);
}

void NetworkOptionsWindowItem::addNetwork(types::NetworkInterface network)
{
    addNetwork(network, network.trustType);
}

void NetworkOptionsWindowItem::addNetworks(QVector<types::NetworkInterface> list)
{
    for (types::NetworkInterface interface : list) {
        addNetwork(interface);
    }
}

void NetworkOptionsWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

void NetworkOptionsWindowItem::onNetworkWhitelistChanged(QVector<types::NetworkInterface> list)
{
    otherNetworksGroup_->updateNetworks(list);
    if (currentNetworkItem_ && currentNetworkItem_ != placeholderItem_) {
        for (types::NetworkInterface interface : list) {
            if (interface.networkOrSsid == currentNetwork_.networkOrSsid && currentNetwork_.trustType != interface.trustType) {
                currentNetworkItem_->setLinkText(trustTypeToString(interface.trustType));
                currentNetwork_.trustType = interface.trustType;
                emit currentNetworkUpdated(currentNetwork_);
            }
        }
    }
    updateShownItems();
}

void NetworkOptionsWindowItem::onLanguageChanged()
{
    desc_->setDescription(tr("Windscribe will auto-disconnect when the device connects to a network tagged \"Unsecured\"."));
    autosecureCheckbox_->setDescription(tr("Mark all newly encountered networks as Secured."));
    autosecureCheckbox_->setCaption(tr("Auto-Secure Networks"));
    placeholderItem_->setTitle(tr("No Network Detected"));

    currentNetworkTitle_->setTitle(tr("CURRENT NETWORK"));
    if (currentNetworkItem_ && currentNetworkItem_ != placeholderItem_) {
        currentNetworkItem_->setLinkText(trustTypeToString(currentNetwork_.trustType));
    }
    otherNetworksTitle_->setTitle(tr("OTHER NETWORKS"));
    otherNetworksGroup_->updateNetworks(preferences_->networkWhiteList());
}

NETWORK_OPTIONS_SCREEN NetworkOptionsWindowItem::getScreen()
{
    return currentScreen_;
}

void NetworkOptionsWindowItem::setScreen(NETWORK_OPTIONS_SCREEN screen)
{
    currentScreen_ = screen;
}

void NetworkOptionsWindowItem::updateShownItems()
{
    if (otherNetworksGroup_->isEmpty()) {
        otherNetworksTitle_->hide(false);
        otherNetworksGroup_->hide(false);
    } else {
        otherNetworksTitle_->show(false);
        otherNetworksGroup_->show(false);
    }
}

void NetworkOptionsWindowItem::onCurrentNetworkClicked()
{
    if (currentNetworkItem_ && currentNetworkItem_ != placeholderItem_) {
        emit networkClicked(currentNetwork_);
    }
}

void NetworkOptionsWindowItem::onAutoSecureChanged(bool enabled)
{
    preferences_->setAutoSecureNetworks(enabled);
}

QString NetworkOptionsWindowItem::trustTypeToString(NETWORK_TRUST_TYPE type)
{
    switch(type)
    {
        case NETWORK_TRUST_SECURED:
            return tr("Secured");
        case NETWORK_TRUST_UNSECURED:
            return tr("Unsecured");
        default:
            return "";
    }
}

} // namespace PreferencesWindow
