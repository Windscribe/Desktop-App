#include "myipmanager.h"

#include <QTimer>

#include "engine/serverapi/serverapi.h"
#include "engine/serverapi/requests/myiprequest.h"
#include "utils/utils.h"

namespace api_resources {


MyIpManager::MyIpManager(QObject *parent, server_api::ServerAPI *serverAPI, INetworkDetectionManager *networkDetectionManager,
                         IConnectStateController *connectStateController) : QObject(parent),
    serverAPI_(serverAPI),
    networkDetectionManager_(networkDetectionManager),
    connectStateController_(connectStateController),
    requestForTimerIsDisconnected_(false),
    curRequest_(nullptr)
{
    connect(connectStateController, &IConnectStateController::stateChanged, this, &MyIpManager::onConnectStateChanged);
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
        SAFE_DELETE(curRequest_);
        curRequest_ = serverAPI_->myIP(kTimeout);
        curRequest_->setProperty("isFromDisconnectedState", connectStateController_->currentState() != CONNECT_STATE_CONNECTED);
        connect(curRequest_, &server_api::BaseRequest::finished, this, &MyIpManager::onMyIpAnswer);
    } else  {
        timer_.stop();
        timer_.start(1000);
    }
}

void MyIpManager::onMyIpAnswer()
{
    QSharedPointer<server_api::MyIpRequest> request(static_cast<server_api::MyIpRequest *>(sender()), &QObject::deleteLater);
    curRequest_ = nullptr;
    if (request->networkRetCode() != SERVER_RETURN_SUCCESS) {
        timer_.stop();
        timer_.start(1000);
    } else {
        emit myIpChanged(request->ip(), request->property("isFromDisconnectedState").toBool());
    }
}

void MyIpManager::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location)
{
    bool isNewStateDisconnected = (state != CONNECT_STATE_CONNECTED);
    if (curRequest_) {
        if (curRequest_->property("isFromDisconnectedState").toBool() != isNewStateDisconnected)
            SAFE_DELETE(curRequest_);
    }
}

} // namespace api_resources
