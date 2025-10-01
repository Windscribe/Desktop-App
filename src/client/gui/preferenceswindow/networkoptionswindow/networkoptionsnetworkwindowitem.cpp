#include "networkoptionsnetworkwindowitem.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

NetworkOptionsNetworkWindowItem::NetworkOptionsNetworkWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), preferences_(preferences), preferencesHelper_(preferencesHelper)
{
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    autoSecureGroup_ = new PreferenceGroup(this);
    autoSecureCheckBox_ = new ToggleItem(autoSecureGroup_);
    autoSecureCheckBox_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/AUTOSECURE"));
    connect(autoSecureCheckBox_, &ToggleItem::stateChanged, this, &NetworkOptionsNetworkWindowItem::onAutoSecureChanged);
    autoSecureGroup_->addItem(autoSecureCheckBox_);
    addItem(autoSecureGroup_);

    preferredProtocolGroup_ = new ProtocolGroup(this,
                                                preferencesHelper,
                                                "",
                                                "preferences/PREFERRED_PROTOCOL",
                                                ProtocolGroup::SelectionType::TOGGLE_SWITCH,
                                                "");
    connect(preferredProtocolGroup_, &ProtocolGroup::connectionModePreferencesChanged, this, &NetworkOptionsNetworkWindowItem::onPreferredProtocolChanged);
    addItem(preferredProtocolGroup_);

    forgetGroup_ = new PreferenceGroup(this);
    forgetItem_ = new LinkItem(forgetGroup_, LinkItem::LinkType::BLANK_LINK);
    forgetGroup_->addItem(forgetItem_);
    addItem(forgetGroup_);
    connect(forgetItem_, &LinkItem::clicked, this, &NetworkOptionsNetworkWindowItem::onForgetClicked);

    connect(preferences, &Preferences::networkWhiteListChanged, this, &NetworkOptionsNetworkWindowItem::onNetworkWhitelistChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &NetworkOptionsNetworkWindowItem::onLanguageChanged);
    onLanguageChanged();
}

void NetworkOptionsNetworkWindowItem::setNetwork(types::NetworkInterface network)
{
    network_ = network;
    autoSecureCheckBox_->setState(network.trustType == NETWORK_TRUST_SECURED);
    preferredProtocolGroup_->setConnectionSettings(preferences_->networkPreferredProtocol(network_.networkOrSsid));

    updateForgetGroup();
}

void NetworkOptionsNetworkWindowItem::onAutoSecureChanged(bool value)
{
    network_.trustType = (value ? NETWORK_TRUST_SECURED : NETWORK_TRUST_UNSECURED);

    QVector<types::NetworkInterface> list = preferences_->networkWhiteList();
    QVector<types::NetworkInterface> out;
    for (types::NetworkInterface interface : list)
    {
        if (interface.networkOrSsid == network_.networkOrSsid)
        {
            interface.trustType = network_.trustType;
        }
        out << interface;
    }
    preferences_->setNetworkWhiteList(out);
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
        if (interface.networkOrSsid == network_.networkOrSsid)
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

void NetworkOptionsNetworkWindowItem::onPreferredProtocolChanged(const types::ConnectionSettings &settings)
{
    preferences_->setNetworkPreferredProtocol(network_.networkOrSsid, settings);
}

QString NetworkOptionsNetworkWindowItem::caption() const
{
    return network_.networkOrSsid;
}

void NetworkOptionsNetworkWindowItem::onLanguageChanged()
{
    autoSecureCheckBox_->setDescription(tr("Windscribe auto-connects if the device connects to this network."));
    autoSecureCheckBox_->setCaption(tr("Auto-Secure"));
    preferredProtocolGroup_->setTitle(tr("Preferred Protocol"));
    preferredProtocolGroup_->setDescription(tr("Choose whether to connect using the recommended tunneling protocol, or to specify a protocol of your choice."));
    forgetItem_->setTitle(tr("Forget Network"));
}

} // namespace PreferencesWindow
