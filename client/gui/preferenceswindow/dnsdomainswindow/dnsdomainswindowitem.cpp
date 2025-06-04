#include "dnsdomainswindowitem.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

DnsDomainsWindowItem::DnsDomainsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences), loggedIn_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    connect(preferences, &Preferences::connectedDnsInfoChanged, this, &DnsDomainsWindowItem::onConnectedDnsPreferencesChanged);

    desc_ = new PreferenceGroup(this);
    desc_->setDescriptionBorderWidth(2);
    addItem(desc_);

    addressesGroup_ = new DnsDomainsGroup(this);
    connect(addressesGroup_, &DnsDomainsGroup::addressesUpdated, this, &DnsDomainsWindowItem::onAddressesUpdated);
    connect(addressesGroup_, &DnsDomainsGroup::setError, this, &DnsDomainsWindowItem::onError);
    connect(addressesGroup_, &DnsDomainsGroup::escape, this, &DnsDomainsWindowItem::escape);
    connect(addressesGroup_, &DnsDomainsGroup::clearError, this, &DnsDomainsWindowItem::onClearError);
    addItem(addressesGroup_);

    setFocusProxy(addressesGroup_);

    addressesGroup_->setAddresses(preferences_->connectedDnsInfo().hostnames);

    setLoggedIn(false);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &DnsDomainsWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString DnsDomainsWindowItem::caption() const
{
    return tr("List of domains");
}

void DnsDomainsWindowItem::setFocusOnTextEntry()
{
    addressesGroup_->setFocusOnTextEntry();
}

void DnsDomainsWindowItem::onAddressesUpdated(const QStringList &addresses)
{
    types::ConnectedDnsInfo curDnsInfo = preferences_->connectedDnsInfo();
    curDnsInfo.hostnames = addresses;
    preferences_->setConnectedDnsInfo(curDnsInfo);

}

void DnsDomainsWindowItem::onError(QString msg)
{
    desc_->setDescription(msg, true);
}

void DnsDomainsWindowItem::onClearError()
{
    desc_->clearError();
}

void DnsDomainsWindowItem::onConnectedDnsPreferencesChanged(const types::ConnectedDnsInfo &dns)
{
    addressesGroup_->setAddresses(preferences_->connectedDnsInfo().hostnames);
}

void DnsDomainsWindowItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;

    if (loggedIn) {
        desc_->clearError();
        desc_->setDescription(tr("Enter the IP and/or wildcards you wish to use for DNS split feature."));
    } else {
        desc_->setDescription(tr("Please log in to modify domains."), true);
    }

    addressesGroup_->setLoggedIn(loggedIn);
}

void DnsDomainsWindowItem::onLanguageChanged()
{
    setLoggedIn(loggedIn_);
}

} // namespace
