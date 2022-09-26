#include "getmyipcontroller.h"

#include <QTimer>

#include "engine/serverapi/serverapi.h"
#include "engine/serverapi/requests/myiprequest.h"
#include "utils/utils.h"

GetMyIPController::GetMyIPController(QObject *parent, server_api::ServerAPI *serverAPI, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    serverAPI_(serverAPI), networkDetectionManager_(networkDetectionManager), requestForTimerIsDisconnected_(false),
    curRequest_(nullptr)
{
    connect(&timer_, &QTimer::timeout, this, &GetMyIPController::onTimer);
    timer_.setSingleShot(true);
}

void GetMyIPController::getIPFromConnectedState(int timeoutMs)
{
    if (timer_.isActive())
        timer_.stop();

    requestForTimerIsDisconnected_ = false;
    timer_.start(timeoutMs);
}

void GetMyIPController::getIPFromDisconnectedState(int timeoutMs)
{
    if (timer_.isActive())
        timer_.stop();

    requestForTimerIsDisconnected_ = true;
    timer_.start(timeoutMs);
}

void GetMyIPController::onTimer()
{
    if (networkDetectionManager_->isOnline()) {
        SAFE_DELETE(curRequest_);
        curRequest_ = serverAPI_->myIP(kTimeout, true);
        curRequest_->setProperty("isFromDisconnectedState", requestForTimerIsDisconnected_);
        connect(curRequest_, &server_api::BaseRequest::finished, this, &GetMyIPController::onMyIpAnswer);
    } else  {
        timer_.stop();
        timer_.start(1000);
    }
}

void GetMyIPController::onMyIpAnswer()
{
    QSharedPointer<server_api::MyIpRequest> request(static_cast<server_api::MyIpRequest *>(sender()), &QObject::deleteLater);
    curRequest_ = nullptr;
    if (request->retCode() != SERVER_RETURN_SUCCESS) {
        timer_.stop();
        timer_.start(1000);
    } else {
        emit answerMyIP(request->ip(), request->property("isFromDisconnectedState").toBool());
    }
}
