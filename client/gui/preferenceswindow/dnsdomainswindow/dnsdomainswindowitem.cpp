#include "dnsdomainswindowitem.h"
#include "utils/logger.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

DnsDomainsWindowItem::DnsDomainsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences), loggedIn_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

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
    //addressesGroup_->setAddresses(preferences->splitTunnelingNetworkRoutes());

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
    ///preferences_->setSplitTunnelingNetworkRoutes(addresses);
    ///emit addressesUpdated(addresses); // tell other screens about change
}

void DnsDomainsWindowItem::onError(QString title, QString msg)
{
    desc_->setDescription(msg, true);
}

void DnsDomainsWindowItem::onClearError()
{
    desc_->clearError();
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
