#include "checkupdatemanager.h"

#include "engine/serverapi/requests/checkupdaterequest.h"
#include "utils/utils.h"

namespace api_resources {

CheckUpdateManager::CheckUpdateManager(QObject *parent, server_api::ServerAPI *serverAPI) : QObject(parent),
    serverAPI_(serverAPI), curRequest_(nullptr)
{
    fetchTimer_ = new QTimer(this);
    connect(fetchTimer_, &QTimer::timeout, this, &CheckUpdateManager::onFetchTimer);
}

CheckUpdateManager::~CheckUpdateManager()
{
    SAFE_DELETE(curRequest_);
}

void CheckUpdateManager::checkUpdate(UPDATE_CHANNEL channel)
{
    fetchTimer_->stop();
    SAFE_DELETE(curRequest_);
    updateChannel_ = channel;
    fetchCheckUpdate();
}

void CheckUpdateManager::onCheckUpdateAnswer()
{
    QSharedPointer<server_api::CheckUpdateRequest> request(static_cast<server_api::CheckUpdateRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        emit checkUpdateUpdated(request->checkUpdate());
        fetchTimer_->start(k24Hours);
    } else {    // on any network error try again in 1 minute
        fetchTimer_->start(kMinute);
    }
    curRequest_ = nullptr;
}

void CheckUpdateManager::onFetchTimer()
{
    fetchCheckUpdate();
}

void CheckUpdateManager::fetchCheckUpdate()
{
    WS_ASSERT(curRequest_ == nullptr);
    curRequest_ = serverAPI_->checkUpdate(updateChannel_);
    connect(curRequest_, &server_api::BaseRequest::finished, this, &CheckUpdateManager::onCheckUpdateAnswer);
}


} // namespace api_resources
