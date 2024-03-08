#include "checkupdatemanager.h"
#include "version/appversion.h"
#include "utils/utils.h"

namespace api_resources {

using namespace wsnet;

CheckUpdateManager::CheckUpdateManager(QObject *parent) : QObject(parent)
{
    fetchTimer_ = new QTimer(this);
    connect(fetchTimer_, &QTimer::timeout, this, &CheckUpdateManager::onFetchTimer);
}

CheckUpdateManager::~CheckUpdateManager()
{
    SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(curRequest_);
}

void CheckUpdateManager::checkUpdate(UPDATE_CHANNEL channel)
{
    fetchTimer_->stop();
    SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(curRequest_);
    updateChannel_ = channel;
    fetchCheckUpdate();
}

void CheckUpdateManager::onCheckUpdateAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::CheckUpdate checkUpdate(jsonData);
        emit checkUpdateUpdated(checkUpdate);
        fetchTimer_->start(k24Hours);
    } else {    // on any network error try again in 1 minute
        fetchTimer_->start(kMinute);
    }
    curRequest_.reset();
}

void CheckUpdateManager::onFetchTimer()
{
    SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(curRequest_);
    fetchCheckUpdate();
}

void CheckUpdateManager::fetchCheckUpdate()
{
    QString osVersion, osBuild;
    Utils::getOSVersionAndBuild(osVersion, osBuild);

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
    {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onCheckUpdateAnswer(serverApiRetCode, jsonData);
        });
    };

    WSNet::instance()->serverAPI()->checkUpdate((UpdateChannel)updateChannel_, AppVersion::instance().version().toStdString(), AppVersion::instance().build().toStdString(),
                                                osVersion.toStdString(), osBuild.toStdString(), callback);
}

} // namespace api_resources
