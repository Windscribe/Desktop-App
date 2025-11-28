#include "splittunnelingaddressesgroup.h"

#include <QPainter>
#include "preferenceswindow/preferencegroup.h"
#include "utils/ipvalidation.h"
#include "addressitem.h"

namespace PreferencesWindow {

SplitTunnelingAddressesGroup::SplitTunnelingAddressesGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), numDomains_(0)
{
    setFlags(flags() | QGraphicsItem::ItemIsFocusable);

    newAddressItem_ = new NewAddressItem(this);
    connect(newAddressItem_, &NewAddressItem::addNewAddressClicked, this, &SplitTunnelingAddressesGroup::onAddClicked);
    connect(newAddressItem_, &NewAddressItem::keyPressed, this, &SplitTunnelingAddressesGroup::clearError);
    connect(newAddressItem_, &NewAddressItem::escape, this, &SplitTunnelingAddressesGroup::escape);
    addItem(newAddressItem_);
}

void SplitTunnelingAddressesGroup::setAddresses(QList<types::SplitTunnelingNetworkRoute> addresses)
{
    if (addresses == addresses_.values()) {
        return;
    }
    for (AddressItem *item : addresses_.keys()) {
        addresses_.remove(item);
        hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    }
    numDomains_ = 0;

    for (types::SplitTunnelingNetworkRoute addr : addresses) {
        addAddressInternal(addr);
    }
    emit addressesUpdated(addresses_.values());
}

void SplitTunnelingAddressesGroup::addAddress(types::SplitTunnelingNetworkRoute &address)
{
    addAddressInternal(address);
    emit addressesUpdated(addresses_.values());
}

void SplitTunnelingAddressesGroup::addAddressInternal(types::SplitTunnelingNetworkRoute &address)
{
    if (IpValidation::isDomain(address.name)) {
        if (numDomains_ >= kMaxDomains) {
            emit setError(tr("There are too many hostnames in the list. Please remove some before adding more."));
            return;
        }
        numDomains_++;
    }

    AddressItem *item = new AddressItem(address, this);
    connect(item, &AddressItem::deleteClicked, this, &SplitTunnelingAddressesGroup::onDeleteClicked);
    connect(item, &AddressItem::activeChanged, this, [this, item](bool active) {
        addresses_[item].active = active;
        emit addressesUpdated(addresses_.values());
    });
    addresses_[item] = address;
    addItem(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    showItems(indexOf(item));
}

void SplitTunnelingAddressesGroup::setFocusOnTextEntry()
{
    newAddressItem_->setSelected(true);
}

void SplitTunnelingAddressesGroup::onAddClicked(QString address)
{
    ValidationCode code = validate(address);
    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE type;
    types::SplitTunnelingNetworkRoute route;

    switch(code) {
    case OK:
        type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME;
        if (IpValidation::isIpCidr(address) || !IpValidation::isValidIpForCidr(address)) {
            type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
        }

        route.name = address;
        route.type = type;
        addAddress(route);
        break;
    case ERROR_EXISTS:
        emit setError(tr("IP or hostname already exists. Please enter a new IP or hostname."));
        break;
    case ERROR_INVALID:
        emit setError(tr("Incorrect IP address/mask combination. Please enter a valid hostname or IP address in plain or CIDR notation."));
        break;
    case ERROR_RESERVED:
        emit setError(tr("This IP address or range is reserved by Windscribe and can not be changed."));
        break;
    }
}

void SplitTunnelingAddressesGroup::onDeleteClicked()
{
    AddressItem *item = static_cast<AddressItem *>(sender());
    if (IpValidation::isDomain(item->getAddressText())) {
        numDomains_--;
    }
    addresses_.remove(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    emit addressesUpdated(addresses_.values());
}

SplitTunnelingAddressesGroup::ValidationCode SplitTunnelingAddressesGroup::validate(QString &address)
{
    if (!IpValidation::isIpCidrOrDomain(address)) {
        return ValidationCode::ERROR_INVALID;
    }

    if (itemByName(address) != nullptr) {
        return ValidationCode::ERROR_EXISTS;
    }

    if (!IpValidation::isValidIpForCidr(address)) {
        return ValidationCode::ERROR_INVALID;
    }

    if (IpValidation::isWindscribeReservedIp(address)) {
        return ValidationCode::ERROR_RESERVED;
    }

    return ValidationCode::OK;
}

AddressItem *SplitTunnelingAddressesGroup::itemByName(QString &address)
{
    for (AddressItem *item: addresses_.keys()) {
        if (item->getAddressText() == address) {
            return item;
        }
    }
    return nullptr;
}

void SplitTunnelingAddressesGroup::setLoggedIn(bool loggedIn)
{
    newAddressItem_->setClickable(loggedIn);
    for (AddressItem *item: addresses_.keys()) {
        item->setClickable(loggedIn);
    }
}

}
