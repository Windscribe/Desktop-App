#include "waitfornetworkconnectivity.h"

WaitForNetworkConnectivity::WaitForNetworkConnectivity(QObject *parent, INetworkDetectionManager *networkDetectionManager) :
    QObject(parent), networkDetectionManager_(networkDetectionManager)
{
    timer_ = new QTimer();
}

void WaitForNetworkConnectivity::wait(int maxTime)
{
    if (networkDetectionManager_->isOnline()) {
        emit connectivityOnline();
    } else {
        connect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &WaitForNetworkConnectivity::onOnlineStateChanged);
        connect(timer_, &QTimer::timeout, this, &WaitForNetworkConnectivity::onTimer);
        timer_->start(maxTime);
    }
}

void WaitForNetworkConnectivity::onOnlineStateChanged(bool isOnline)
{
    if (isOnline) {
        disconnect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &WaitForNetworkConnectivity::onOnlineStateChanged);
        disconnect(timer_, &QTimer::timeout, this, &WaitForNetworkConnectivity::onTimer);
        emit connectivityOnline();
    }
}

void WaitForNetworkConnectivity::onTimer()
{
    disconnect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &WaitForNetworkConnectivity::onOnlineStateChanged);
    disconnect(timer_, &QTimer::timeout, this, &WaitForNetworkConnectivity::onTimer);
    timer_->stop();
    emit timeoutExpired();
}
