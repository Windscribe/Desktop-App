#include "myipmanager.h"

#include <QTimer>

#include "utils/utils.h"
#include "api_responses/myip.h"

namespace api_resources {

using namespace wsnet;

MyIpManager::MyIpManager(QObject *parent, INetworkDetectionManager *networkDetectionManager,
                         IConnectStateController *connectStateController) : QObject(parent),
    networkDetectionManager_(networkDetectionManager),
    connectStateController_(connectStateController)
{
    connect(&timer_, &QTimer::timeout, this, &MyIpManager::onTimer);
    timer_.setSingleShot(true);
}

void MyIpManager::getIP(int timeoutMs)
{
    if (timer_.isActive())
        timer_.stop();
    timer_.start(timeoutMs);
}

void MyIpManager::onTimer()
{
    if (networkDetectionManager_->isOnline()) {
        SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(curRequest_);

        auto connectStateWatcher = new ConnectStateWatcher(this, connectStateController_);
        bool isFromDisconnectedState = connectStateController_->currentState() != CONNECT_STATE_CONNECTED;

        auto callback = [this, connectStateWatcher, isFromDisconnectedState](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
        {
            QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData, connectStateWatcher, isFromDisconnectedState] { // NOLINT: false positive for memory leak
                onMyIpAnswer(serverApiRetCode, jsonData, connectStateWatcher, isFromDisconnectedState);
            });
        };

        curRequest_ = WSNet::instance()->serverAPI()->myIP(callback);
    } else  {
        timer_.stop();
        timer_.start(1000);
    }
}

void MyIpManager::onMyIpAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData, ConnectStateWatcher *connectStateWatcher, bool isFromDisconnectedState)
{
    if (connectStateWatcher->isVpnConnectStateChanged())
        return;

    if (curRequest_)
        curRequest_.reset();

    if (serverApiRetCode != ServerApiRetCode::kSuccess) {
        timer_.stop();
        timer_.start(1000);
    } else {
        api_responses::MyIp myIp(jsonData);
        emit myIpChanged(myIp.ip(), isFromDisconnectedState);
    }
}

} // namespace api_resources
