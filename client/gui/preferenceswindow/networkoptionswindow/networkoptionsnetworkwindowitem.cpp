#include "networkoptionsnetworkwindowitem.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "networkoptionsshared.h"
#include "preferenceswindow/preferencegroup.h"
#include "utils/logger.h"

namespace PreferencesWindow {

NetworkOptionsNetworkWindowItem::NetworkOptionsNetworkWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), preferences_(preferences), preferencesHelper_(preferencesHelper)
{
    setSpacerHeight(PREFERENCES_MARGIN);

    autoSecureGroup_ = new PreferenceGroup(this, tr("Windscribe auto-connects if the device connects to this network."), "");
    autoSecureCheckBox_ = new CheckBoxItem(autoSecureGroup_, tr("Auto-Secure"), QString());
    autoSecureCheckBox_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/AUTOSECURE"));
    connect(autoSecureCheckBox_, &CheckBoxItem::stateChanged, this, &NetworkOptionsNetworkWindowItem::onAutoSecureChanged);
    autoSecureGroup_->addItem(autoSecureCheckBox_);
    addItem(autoSecureGroup_);

    preferredProtocolGroup_ = new ProtocolGroup(this,
                                                preferencesHelper,
                                                tr("Preferred Protocol"),
                                                "preferences/PREFERRED_PROTOCOL",
                                                ProtocolGroup::SelectionType::TOGGLE_SWITCH,
                                                tr("Choose whether to connect using the recommended tunneling protocol, or to specify a protocol of your choice."), "");
    addItem(preferredProtocolGroup_);

    forgetGroup_ = new PreferenceGroup(this);
    forgetItem_ = new LinkItem(forgetGroup_, LinkItem::LinkType::BLANK_LINK, tr("Forget Network"));
    forgetGroup_->addItem(forgetItem_);
    addItem(forgetGroup_);
    connect(forgetItem_, &LinkItem::clicked, this, &NetworkOptionsNetworkWindowItem::onForgetClicked);

    //connect(&LanguageController::instance(), &LanguageController::languageChanged(), this, &NetworkOptionsNetworkWindowItem::onLanguageChanged);
    connect(preferences, &Preferences::networkWhiteListChanged, this, &NetworkOptionsNetworkWindowItem::onNetworkWhitelistChanged);
}

void NetworkOptionsNetworkWindowItem::setNetwork(types::NetworkInterface network)
{
    network_ = network;
    autoSecureCheckBox_->setState(network.trustType == NETWORK_TRUST_SECURED);

    // TODO per-network connection mode

    updateForgetGroup();
}

void NetworkOptionsNetworkWindowItem::onAutoSecureChanged(bool value)
{
    network_.trustType = (value ? NETWORK_TRUST_SECURED : NETWORK_TRUST_UNSECURED);

    QVector<types::NetworkInterface> list = preferences_->networkWhiteList();
    for (types::NetworkInterface interface : list)
    {
        if (interface.friendlyName == network_.friendlyName)
        {
            interface.trustType = network_.trustType;
        }
        list << interface;
    }
    preferences_->setNetworkWhiteList(list);
}

void NetworkOptionsNetworkWindowItem::onNetworkWhitelistChanged(QVector<types::NetworkInterface> list)
{
    for (types::NetworkInterface interface : list)
    {
        if (interface.networkOrSsid == network_.networkOrSsid)
        {
            network_.trustType = interface.trustType;
            autoSecureCheckBox_->setState(network_.trustType == NETWORK_TRUST_SECURED);
        }
    }
}

void NetworkOptionsNetworkWindowItem::onForgetClicked()
{
    QVector<types::NetworkInterface> entries;
    QVector<types::NetworkInterface> list = preferences_->networkWhiteList();
    for (types::NetworkInterface interface : list)
    {
        if (interface.friendlyName == network_.friendlyName)
        {
            continue;
        }
        entries << interface;
    }
    preferences_->setNetworkWhiteList(entries);
    emit escape();
}

void NetworkOptionsNetworkWindowItem::setCurrentNetwork(types::NetworkInterface network)
{
    currentNetwork_ = network;
    updateForgetGroup();
}

void NetworkOptionsNetworkWindowItem::updateForgetGroup()
{
    if (currentNetwork_.networkOrSsid == network_.networkOrSsid)
    {
        forgetGroup_->hide(false);
    }
    else
    {
        forgetGroup_->show(false);
    }
}

} // namespace PreferencesWindow
