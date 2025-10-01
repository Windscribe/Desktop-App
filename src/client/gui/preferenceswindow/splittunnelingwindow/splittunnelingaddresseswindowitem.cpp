#include "splittunnelingaddresseswindowitem.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

SplitTunnelingAddressesWindowItem::SplitTunnelingAddressesWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences), loggedIn_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    desc_ = new PreferenceGroup(this);
    desc_->setDescriptionBorderWidth(2);
    addItem(desc_);

    addressesGroup_ = new SplitTunnelingAddressesGroup(this);
    connect(addressesGroup_, &SplitTunnelingAddressesGroup::addressesUpdated, this, &SplitTunnelingAddressesWindowItem::onAddressesUpdated);
    connect(addressesGroup_, &SplitTunnelingAddressesGroup::setError, this, &SplitTunnelingAddressesWindowItem::onError);
    connect(addressesGroup_, &SplitTunnelingAddressesGroup::escape, this, &SplitTunnelingAddressesWindowItem::escape);
    connect(addressesGroup_, &SplitTunnelingAddressesGroup::clearError, this, &SplitTunnelingAddressesWindowItem::onClearError);

    addItem(addressesGroup_);
    setFocusProxy(addressesGroup_);

    connect(preferences_, &Preferences::splitTunnelingChanged, this, &SplitTunnelingAddressesWindowItem::onPreferencesChanged);
    onPreferencesChanged();

    setLoggedIn(false);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SplitTunnelingAddressesWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString SplitTunnelingAddressesWindowItem::caption() const
{
    return tr("IPs & Hostnames");
}

void SplitTunnelingAddressesWindowItem::setFocusOnTextEntry()
{
    addressesGroup_->setFocusOnTextEntry();
}

void SplitTunnelingAddressesWindowItem::onAddressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses)
{
    preferences_->setSplitTunnelingNetworkRoutes(addresses);
    emit addressesUpdated(addresses); // tell other screens about change
}

void SplitTunnelingAddressesWindowItem::onError(QString msg)
{
    desc_->setDescription(msg, true);
}

void SplitTunnelingAddressesWindowItem::onClearError()
{
    desc_->clearError();
}

void SplitTunnelingAddressesWindowItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;

    if (loggedIn) {
        desc_->clearError();
        desc_->setDescription(tr("Enter the IP and/or hostnames you wish to include in or exclude from the VPN tunnel below."));
    } else {
        desc_->setDescription(tr("Please log in to modify split tunneling rules."), true);
    }

    addressesGroup_->setLoggedIn(loggedIn);
}

void SplitTunnelingAddressesWindowItem::onLanguageChanged()
{
    setLoggedIn(loggedIn_);
}

void SplitTunnelingAddressesWindowItem::onPreferencesChanged()
{
    addressesGroup_->setAddresses(preferences_->splitTunnelingNetworkRoutes());
}

} // namespace
