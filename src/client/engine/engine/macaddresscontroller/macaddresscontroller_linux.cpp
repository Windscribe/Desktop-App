#include "macaddresscontroller_linux.h"

#include "engine/networkdetectionmanager/networkdetectionmanager_linux.h"
#include "utils/linuxutils.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"

MacAddressController_linux::MacAddressController_linux(QObject *parent, INetworkDetectionManager *ndManager, Helper *helper)
    : IMacAddressController(parent), helper_(helper)
{
    networkDetectionManager_ = dynamic_cast<NetworkDetectionManager_linux *>(ndManager);
    connect(networkDetectionManager_, &NetworkDetectionManager_linux::networkListChanged, this, &MacAddressController_linux::onNetworkListChanged);
    networkDetectionManager_->getCurrentNetworkInterface(lastInterface_);

#ifdef CLI_ONLY
    spoofInProgress_ = false;
#endif
}

MacAddressController_linux::~MacAddressController_linux()
{
}

void MacAddressController_linux::initMacAddrSpoofing(const types::MacAddrSpoofing &spoofing)
{
    spoofing_ = spoofing;
    refreshSelectedInterface();

    if (spoofing.isEnabled) {
        enableSpoofing(spoofing_.isAutoRotate);
    }

    // Echo the refreshed selection so the frontend persists it.
    emit macAddrSpoofingChanged(spoofing_);
}

void MacAddressController_linux::setMacAddrSpoofing(const types::MacAddrSpoofing &spoofing)
{
    if (spoofing == spoofing_) {
        return;
    }

    qCInfo(LOG_BASIC) << "MacAddressController_linux::setMacAddrSpoofing MacAddrSpoofing has changed.";
    qCInfo(LOG_BASIC) << "MacAddrSpoofing settings:" << spoofing.toJson(true);

    if (spoofing.isEnabled) {
        bool autoRotateChanged = (spoofing_.isAutoRotate != spoofing.isAutoRotate);
        spoofing_ = spoofing;
        enableSpoofing(autoRotateChanged);
    } else if (spoofing_.isEnabled && !spoofing.isEnabled) {
        spoofing_ = spoofing;
        disableSpoofing();
    }

    emit macAddrSpoofingChanged(spoofing_);
}

void MacAddressController_linux::enableSpoofing(bool networkChanged)
{
#ifndef CLI_ONLY
    // Spoofing is applied via nmcli, so it can't work while NetworkManager isn't running. Disable it (do NOT
    // skip-and-retry): the GUI greys the control on the same isNetworkManagerActive() check and tells the
    // user it's unavailable, so the engine must not keep the setting armed and silently spoof the moment NM
    // comes back -- that would be applying behind a greyed-out control. NM not running => spoofing off, in
    // both the engine and the UI. The user re-enables it once NM is back.
    if (!LinuxUtils::isNetworkManagerActive()) {
        qCInfo(LOG_BASIC) << "MAC spoofing disabled: NetworkManager is not active";
        spoofing_.isEnabled = false;
        emit macAddrSpoofingChanged(spoofing_);
        return;
    }
#endif

    types::NetworkInterface interface;
    networkDetectionManager_->getCurrentNetworkInterface(interface);

    if (interface.interfaceName != spoofing_.selectedNetworkInterface.interfaceName) {
        // Current interface is not the spoofed interface, ignore
#ifdef CLI_ONLY
        helper_->resetMacAddresses(spoofing_.selectedNetworkInterface.interfaceName);
#else
        helper_->resetMacAddresses(spoofing_.selectedNetworkInterface.networkOrSsid);
#endif
        return;
    }

    qCInfo(LOG_BASIC) << "Enabling spoofing" << (networkChanged ? "(network changed)" : "");

    if (networkChanged && spoofing_.isAutoRotate) {
        qCInfo(LOG_BASIC) << "Applying auto rotate";
        spoofing_.macAddress = NetworkUtils::generateRandomMacAddress();
        emit macAddrSpoofingChanged(spoofing_);
    } else {
        if (interface.physicalAddress == NetworkUtils::formatMacAddress(spoofing_.macAddress)) {
            // MAC is already set correctly, do nothing.
            return;
        }
        qCInfo(LOG_BASIC) << "Applying manual MAC address";
        if (spoofing_.macAddress.isEmpty()) {
            spoofing_.macAddress = NetworkUtils::generateRandomMacAddress();
            emit macAddrSpoofingChanged(spoofing_);
        }
    }

    lastInterface_ = interface;
    applySpoof(interface, spoofing_.macAddress);
}

void MacAddressController_linux::disableSpoofing()
{
    qCInfo(LOG_BASIC) << "Disabling spoofing";
    removeSpoofs();
}

void MacAddressController_linux::onNetworkListChanged(const QList<types::NetworkInterface> &interfaces)
{
    spoofing_.networkInterfaces = interfaces;
    // A Wi-Fi adapter is missing from the list whenever it is not associated (suspend, NetworkManager restart),
    // so an absent adapter keeps its selection and spoofing is reapplied when it returns.
    refreshSelectedInterface();

    emit macAddrSpoofingChanged(spoofing_);

    if (spoofing_.isEnabled) {
        types::NetworkInterface interface;
        networkDetectionManager_->getCurrentNetworkInterface(interface);

        if (interface.interfaceIndex == lastInterface_.interfaceIndex && interface.networkOrSsid == lastInterface_.networkOrSsid) {
            // Same interface as before
#ifdef CLI_ONLY
            if (spoofInProgress_) {
                spoofInProgress_ = false;
            }
#endif
            return;
        }

        if (interface.isNoNetworkInterface() || interface.networkOrSsid.isEmpty()) {
            // No interface or disconnected network
#ifdef CLI_ONLY
            // For CLI only, the helper brings down the interface, changes the MAC, and then brings it back up.  We ignore 'no interface' events until this is done.
            if (!spoofInProgress_) {
#endif
                lastInterface_ = interface;
#ifdef CLI_ONLY
            }
#endif
            return;
        }


#ifdef CLI_ONLY
        // if we got this far, then an actual interface change interrupted the spoofing operation.  Reset the flag.
        spoofInProgress_ = false;
#endif

        if (spoofing_.selectedNetworkInterface.interfaceName != interface.interfaceName) {
            removeSpoofs();
        } else {
            enableSpoofing(true);
        }
    }
}

void MacAddressController_linux::applySpoof(const types::NetworkInterface &interface, const QString &macAddress)
{
    if (macAddress.isEmpty()) {
        return;
    }

    if (types::NetworkInterface::isNoNetworkInterface(interface.interfaceIndex)) {
        qCInfo(LOG_BASIC) << "MacAddressController_linux::applySpoof Skipping (no interface selected)";
        return;
    }

    if (interface.interfaceName != spoofing_.selectedNetworkInterface.interfaceName) {
        qCInfo(LOG_BASIC) << "MacAddressController_linux::applySpoof Skipping (Selected interface is different than current)";
        return;
    }

#ifndef CLI_ONLY
    // Re-check right before the helper call: NetworkManager may have stopped since enableSpoofing checked
    // (e.g. a restart). Disable, matching enableSpoofing and the greyed GUI, so the engine never spoofs while
    // the UI shows the feature unavailable. No hard-failure warning here -- NM being down is not a spoof
    // failure, it's the feature being unavailable.
    if (!LinuxUtils::isNetworkManagerActive()) {
        qCInfo(LOG_BASIC) << "MacAddressController_linux::applySpoof disabling (NetworkManager is not active)";
        spoofing_.isEnabled = false;
        emit macAddrSpoofingChanged(spoofing_);
        return;
    }
#endif

    if (!helper_->setMacAddress(interface.interfaceName, macAddress, interface.networkOrSsid, interface.interfaceType == NETWORK_INTERFACE_WIFI)) {
        qCWarning(LOG_BASIC) << "MacAddressController_linux::applySpoof failed.";
        // Disable spoofing, otherwise the network events from the failed attempt's interface bounce re-arm
        // an endless retry/flap loop.
        spoofing_.isEnabled = false;
        emit macAddrSpoofingChanged(spoofing_);
        emit sendUserWarning(USER_WARNING_MAC_SPOOFING_FAILURE_HARD);
        return;
    }

#ifdef CLI_ONLY
    spoofInProgress_ = true;
#endif
}

void MacAddressController_linux::removeSpoofs()
{
    helper_->resetMacAddresses();
}

void MacAddressController_linux::refreshSelectedInterface()
{
    // Matched by name: an adapter's index can change when its driver reloads (e.g. on resume or reboot).
    for (const types::NetworkInterface &interface : std::as_const(spoofing_.networkInterfaces)) {
        if (interface.interfaceName == spoofing_.selectedNetworkInterface.interfaceName) {
            spoofing_.selectedNetworkInterface = interface;
            return;
        }
    }

    // Keep an absent adapter in the list so Preferences keeps showing it while spoofing stays set for it.
    if (!types::NetworkInterface::isNoNetworkInterface(spoofing_.selectedNetworkInterface.interfaceIndex)) {
        spoofing_.networkInterfaces.append(spoofing_.selectedNetworkInterface);
    }
}
