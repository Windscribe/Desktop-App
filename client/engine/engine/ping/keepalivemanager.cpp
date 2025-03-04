#include "keepalivemanager.h"

#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/hardcodedsettings.h"
#include "utils/utils.h"

using namespace wsnet;

KeepAliveManager::KeepAliveManager(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    isEnabled_(false), curConnectState_(CONNECT_STATE_DISCONNECTED)
{
    connect(stateController, &IConnectStateController::stateChanged, this, &KeepAliveManager::onConnectStateChanged);
    connect(&timer_, &QTimer::timeout, this, &KeepAliveManager::onTimer);
}

void KeepAliveManager::setEnabled(bool isEnabled)
{
    isEnabled_ = isEnabled;
    if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_)
        doDnsRequest();
    else
        timer_.stop();
}

void KeepAliveManager::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location)
{
    Q_UNUSED(reason);
    Q_UNUSED(err);
    Q_UNUSED(location);

    curConnectState_ = state;
    if (state == CONNECT_STATE_CONNECTED && isEnabled_)
        doDnsRequest();
    else
        timer_.stop();
}

void KeepAliveManager::onTimer()
{
    using namespace std::placeholders;

    for (int i = 0; i < ips_.count(); ++i) {
        if (!ips_[i].bFailed_) {
            WSNet::instance()->pingManager()->ping(ips_[i].ip_.toStdString(), std::string(), wsnet::PingType::kIcmp,
                                                   std::bind(&KeepAliveManager::onPingFinished, this, _1, _2, _3, _4));
            return;
        }
    }
    // if all failed then select random
    if (ips_.count() > 0) {
        int ind = Utils::generateIntegerRandom(0, ips_.count() - 1);
        WSNet::instance()->pingManager()->ping(ips_[ind].ip_.toStdString(), std::string(), wsnet::PingType::kIcmp,
                                               std::bind(&KeepAliveManager::onPingFinished, this, _1, _2, _3, _4));
    }
}

void KeepAliveManager::onDnsRequestFinished(std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
{
    if (result->error()->isSuccess()) {
        ips_.clear();
        for (const auto &ip : result->ips()) {
            ips_ << IP_DESCR(QString::fromStdString(ip));
        }
        if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_) {
            timer_.start(KEEP_ALIVE_TIMEOUT);
        }
    } else {
        if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_) {
            doDnsRequest();
        }
    }
}

void KeepAliveManager::onPingFinished(const std::string &ip, bool isSuccess, int32_t timeMs, bool isFromDisconnectedVpnState)
{
    Q_UNUSED(isSuccess);
    Q_UNUSED(isFromDisconnectedVpnState);

    QString ipStr = QString::fromStdString(ip);
    for (int i = 0; i < ips_.count(); ++i) {
        if (ips_[i].ip_ == ipStr) {
            ips_[i].bFailed_ = !isSuccess;
            break;
        }
    }
}

void KeepAliveManager::doDnsRequest()
{
    auto callback = [this] (std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
    {
        QMetaObject::invokeMethod(this, [this, requestId, hostname, result] { // NOLINT: false positive for memory leak
            onDnsRequestFinished(requestId, hostname, result);
        });
    };
    WSNet::instance()->dnsResolver()->lookup(HardcodedSettings::instance().windscribeServerUrl().toStdString(), 0, callback);
}
