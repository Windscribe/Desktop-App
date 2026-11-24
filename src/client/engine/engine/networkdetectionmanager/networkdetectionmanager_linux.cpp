#include "networkdetectionmanager_linux.h"

#include <QRegularExpression>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <linux/wireless.h>

#include "utils/log/categories.h"
#include "utils/log/debug_trace.h"
#include "utils/network_utils/network_utils_linux.h"
#include "utils/utils.h"

const int typeIdNetworkInterface = qRegisterMetaType<types::NetworkInterface>("types::NetworkInterface");

NetworkDetectionManager_linux::NetworkDetectionManager_linux(QObject *parent, Helper *helper) : INetworkDetectionManager(parent),
    defaultRoute_([] {
        DefaultRoute_linux route;
        QString adapterIp;
        NetworkUtils_linux::getDefaultRoute(route.gateway, route.interface, adapterIp, true);
        return route;
    })
{
    WS_DEBUG_TRACE_SCOPE("NetworkDetectionManager_linux::ctor");
    Q_UNUSED(helper);

    networkInterface_ = types::NetworkInterface::noNetworkInterface();
    getDefaultRouteInterface(isOnline_);
    updateNetworkInfo(false);
    onRoutesChanged();

    routeMonitorThread_ = new QThread;
    routeMonitor_ = new RouteMonitor_linux;
    connect(routeMonitor_, &RouteMonitor_linux::routesChanged, this, &NetworkDetectionManager_linux::onRoutesChanged);
    connect(routeMonitor_, &RouteMonitor_linux::tunnelRoutesChanged, this, &NetworkDetectionManager_linux::onTunnelRoutesChanged);
    connect(routeMonitorThread_, &QThread::started, routeMonitor_, &RouteMonitor_linux::init);
    connect(routeMonitorThread_, &QThread::finished, routeMonitor_, &RouteMonitor_linux::finish);
    connect(routeMonitorThread_, &QThread::finished, routeMonitor_, &RouteMonitor_linux::deleteLater);
    routeMonitor_->moveToThread(routeMonitorThread_);
    routeMonitorThread_->start(QThread::LowPriority);
}

NetworkDetectionManager_linux::~NetworkDetectionManager_linux()
{
    if (routeMonitorThread_) {
        routeMonitorThread_->quit();
        routeMonitorThread_->wait();
        routeMonitorThread_->deleteLater();
    }
}

void NetworkDetectionManager_linux::getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate)
{
    Q_UNUSED(forceUpdate);
    networkInterface = networkInterface_;
}

bool NetworkDetectionManager_linux::isOnline()
{
    return isOnline_;
}

void NetworkDetectionManager_linux::onRoutesChanged()
{
    updateNetworkInfo(true);

    emit networkListChanged(NetworkUtils_linux::currentNetworkInterfaces(true));
}

void NetworkDetectionManager_linux::onTunnelRoutesChanged()
{
    if (defaultRoute_.differsFromLastRead()) {
        onRoutesChanged();
    }
}

void NetworkDetectionManager_linux::updateNetworkInfo(bool bWithEmitSignal)
{
    WS_DEBUG_TRACE_SCOPE("NetworkDetectionManager_linux::updateNetworkInfo");
    bool newIsOnline;
    QString ifname = getDefaultRouteInterface(newIsOnline);

    if (isOnline_ != newIsOnline) {
        isOnline_ = newIsOnline;
        emit onlineStateChanged(isOnline_);
    }

    types::NetworkInterface newNetworkInterface = NetworkUtils_linux::networkInterfaceByName(ifname);
    if (newNetworkInterface != networkInterface_) {
        bool significantChange = false;

        if (newNetworkInterface.interfaceName != networkInterface_.interfaceName) {
            significantChange = true;
        } else if (newNetworkInterface.networkOrSsid != networkInterface_.networkOrSsid) {
            significantChange = true;
        } else {
            qCInfo(LOG_BASIC) << "Minor interface change (e.g. same-SSID AP roam), skipping reconnect";
        }

        networkInterface_ = newNetworkInterface;
        if (bWithEmitSignal && significantChange) {
            emit networkChanged(networkInterface_);
        }
    }
}

QString NetworkDetectionManager_linux::getDefaultRouteInterface(bool &isOnline)
{
    const DefaultRoute_linux route = defaultRoute_.read();
    if (route.gateway.isEmpty() || route.interface.isEmpty()) {
        isOnline = false;
        return QString();
    }
    isOnline = true;
    return route.interface;
}


