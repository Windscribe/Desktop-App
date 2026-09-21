#include "networkdetectionmanager_win.h"

#include <memory>
#include <optional>

#include "engine/sleepevents/isleepevents.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils_win.h"

namespace {

class NetworkDetectionManager_winDataSource final : public INetworkDetectionManager_winDataSource
{
public:
    types::NetworkInterface currentNetworkInterface() override
    {
        return NetworkUtils_win::currentNetworkInterface();
    }

    QString currentNetworkInterfaceGuid() override
    {
        return NetworkUtils_win::currentNetworkInterfaceGuid();
    }

    bool isNetworkUnidentified(const QString &interfaceGuid) override
    {
        return NetworkUtils_win::isNetworkUnidentified(interfaceGuid);
    }

    bool isOnline() override
    {
        return NetworkUtils_win::haveActiveInterface() || NetworkUtils_win::haveInternetConnectivity().value_or(false);
    }

    std::optional<QString> networkIdFromInterfaceGuid(const QString &interfaceGuid) override
    {
        return NetworkUtils_win::networkIdFromInterfaceGuid(interfaceGuid);
    }

    void refreshNetworkInterfaces() override
    {
        NetworkUtils_win::currentNetworkInterfaces(false, true);
    }
};

} // namespace

NetworkDetectionManager_win::NetworkDetectionManager_win(QObject *parent, ISleepEvents *sleepEvents)
    : NetworkDetectionManager_win(parent, std::make_unique<NetworkDetectionManager_winDataSource>(), true, sleepEvents)
{
}

NetworkDetectionManager_win::NetworkDetectionManager_win(
    QObject *parent, std::unique_ptr<INetworkDetectionManager_winDataSource> dataSource,
    bool startWorker, ISleepEvents *sleepEvents)
    : INetworkDetectionManager(parent),
      dataSource_(std::move(dataSource))
{
    curNetworkInterface_ = dataSource_->currentNetworkInterface();
    curNetworkId_ = dataSource_->networkIdFromInterfaceGuid(curNetworkInterface_.interfaceGuid);
    bLastIsOnline_ = isOnlineImpl();

    if (sleepEvents) {
        connect(sleepEvents, &ISleepEvents::gotoWake, this, &NetworkDetectionManager_win::onWake, Qt::QueuedConnection);
    }

    if (!startWorker) {
        return;
    }

    networkWorker_ = new NetworkChangeWorkerThread(this);
    connect(networkWorker_, &NetworkChangeWorkerThread::finished, networkWorker_, &QObject::deleteLater);
    connect(networkWorker_, &NetworkChangeWorkerThread::networkChanged,
            this, &NetworkDetectionManager_win::onNetworkChanged, Qt::QueuedConnection);

    networkWorker_->start();
}

NetworkDetectionManager_win::~NetworkDetectionManager_win()
{
    if (networkWorker_) {
        networkWorker_->earlyExit();
        networkWorker_->wait();
    }
}

void NetworkDetectionManager_win::onWake()
{
    needsPostWakeRefresh_ = true;
}

void NetworkDetectionManager_win::onNetworkChanged()
{
    bool bCurIsOnline = isOnlineImpl();
    if (bLastIsOnline_ != bCurIsOnline) {
        bLastIsOnline_ = bCurIsOnline;
        emit onlineStateChanged(bLastIsOnline_);
    }

    // Check if the current interface or its network changed, without updating the list of interfaces.
    // Doing this avoids e.g. repopulating SSIDs, which causes a location request in Windows 11 24H2 and later.
    // The network id catches joining a different network on the same adapter, e.g. across a sleep/wake.
    QString guid = dataSource_->currentNetworkInterfaceGuid();
    std::optional<QString> networkId = dataSource_->networkIdFromInterfaceGuid(curNetworkInterface_.interfaceGuid);
    bool networkIdChanged;
    if (networkId.has_value()) {
        // No trusted baseline means the network may have changed while the id was unavailable; refresh to resync.
        networkIdChanged = !curNetworkId_.has_value() || *networkId != *curNetworkId_;
        refreshedOnMissingId_ = false;
    } else {
        // A missing id can hide a real change. Refresh once per streak of missing ids: Wi-Fi names come from
        // the WLAN service, so the refresh still gets the right name when the network list has no answer.
        networkIdChanged = !refreshedOnMissingId_;
        refreshedOnMissingId_ = true;
    }
    if (needsPostWakeRefresh_) {
        // The OS network list still holds the pre-sleep answer for a while after a wake, so refreshing
        // early yields the old name. Once it is online and identified, refresh even if nothing looks
        // changed: the cached interface and id pair cannot be trusted across a sleep.
        std::optional<QString> currentNetworkId = dataSource_->networkIdFromInterfaceGuid(guid);
        if (!bCurIsOnline || !currentNetworkId.has_value() || dataSource_->isNetworkUnidentified(guid)) {
            return;
        }
        needsPostWakeRefresh_ = false;
    } else if (curNetworkInterface_.active && guid == curNetworkInterface_.interfaceGuid && !networkIdChanged) {
        return;
    }

    // Now that we know the interface changed (or post-wake settlement occurred), force an update
    dataSource_->refreshNetworkInterfaces();

    curNetworkInterface_ = dataSource_->currentNetworkInterface();
    // Key the id to the interface stored above so the pair can never describe two different adapters.
    curNetworkId_ = dataSource_->networkIdFromInterfaceGuid(curNetworkInterface_.interfaceGuid);
    // If the id is missing here, this refresh already counts as the missing-id streak's one refresh.
    refreshedOnMissingId_ = !curNetworkId_.has_value();

    // If still online, but current interface is "no interface", don't emit the signal
    // In theory this should never happen, since we exclude app VPN interfaces when getting the current interface,
    // but this code has historically been here and doesn't seem to cause any problems.
    if ((curNetworkInterface_.interfaceIndex == -1 && !bLastIsOnline_) || curNetworkInterface_.interfaceIndex != -1) {
        emit networkChanged(curNetworkInterface_);
    }
}

bool NetworkDetectionManager_win::isOnlineImpl()
{
    return dataSource_->isOnline();
}

void NetworkDetectionManager_win::getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate)
{
    Q_UNUSED(forceUpdate);
    networkInterface = curNetworkInterface_;
}

bool NetworkDetectionManager_win::isOnline()
{
    return bLastIsOnline_;
}
