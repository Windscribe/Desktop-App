#include "getwireguardconfiginloop.h"
#include "utils/utils.h"

GetWireGuardConfigInLoop::GetWireGuardConfigInLoop(QObject *parent, ServerAPI *serverAPI, uint serverApiUserRole) :
    QObject(parent), serverAPI_(serverAPI), serverApiUserRole_(serverApiUserRole), getConfig_(nullptr)
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

    getConfig_ = new GetWireGuardConfig(this, serverAPI_, serverApiUserRole_);
    connect(getConfig_, &GetWireGuardConfig::getWireGuardConfigAnswer, this, &GetWireGuardConfigInLoop::onGetWireGuardConfigAnswer);
    onFetchWireGuardConfig();
}

void GetWireGuardConfigInLoop::stop()
{
    fetchWireguardConfigTimer_->stop();
    SAFE_DELETE_LATER(getConfig_);
}

void GetWireGuardConfigInLoop::onGetWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, const WireGuardConfig &config)
{
    if (retCode == SERVER_RETURN_SUCCESS || retCode == SERVER_RETURN_WIREGUARD_KEY_LIMIT)
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
