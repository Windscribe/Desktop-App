#include "getwireguardconfiginloop.h"
#include "utils/utils.h"

GetWireGuardConfigInLoop::GetWireGuardConfigInLoop(QObject *parent, server_api::ServerAPI *serverAPI) :
    QObject(parent), serverAPI_(serverAPI), getConfig_(nullptr)
{
    fetchWireguardConfigTimer_ = new QTimer(this);
    connect(fetchWireguardConfigTimer_, SIGNAL(timeout()), SLOT(onFetchWireGuardConfig()));
}

void GetWireGuardConfigInLoop::getWireGuardConfig(const QString &serverName, bool deleteOldestKey, const QString &deviceId)
{
    stop();
    serverName_ = serverName;
    deleteOldestKey_ = deleteOldestKey;
    deviceId_ = deviceId;

    getConfig_ = new GetWireGuardConfig(this, serverAPI_);
    connect(getConfig_, &GetWireGuardConfig::getWireGuardConfigAnswer, this, &GetWireGuardConfigInLoop::onGetWireGuardConfigAnswer);
    onFetchWireGuardConfig();
}

void GetWireGuardConfigInLoop::stop()
{
    fetchWireguardConfigTimer_->stop();
    SAFE_DELETE_LATER(getConfig_);
}

void GetWireGuardConfigInLoop::onGetWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config)
{
    if (retCode == WireGuardConfigRetCode::kSuccess || retCode == WireGuardConfigRetCode::kKeyLimit)
    {
        stop();
        emit getWireGuardConfigAnswer(retCode, config);
    }
    else
    {
        // try get config again after 2 sec
        fetchWireguardConfigTimer_->start(LOOP_PERIOD);
    }
}

void GetWireGuardConfigInLoop::onFetchWireGuardConfig()
{
    getConfig_->getWireGuardConfig(serverName_, deleteOldestKey_, deviceId_);
}
