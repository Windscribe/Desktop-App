#include "pingmanager.h"

#include <QTimeZone>

#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "types/pingtime.h"
#include "utils/extraconfig.h"
#include "utils/utils.h"

using namespace wsnet;

PingManager::PingManager(QObject *parent, IConnectStateController *stateController,
                         INetworkDetectionManager *networkDetectionManager, const QString &storageSettingName) : QObject(parent),
    connectStateController_(stateController), networkDetectionManager_(networkDetectionManager), pingStorage_(storageSettingName)
{
    isLogPings_ = ExtraConfig::instance().getLogPings();
    isUseIcmpPings_ = ExtraConfig::instance().getUseICMPPings();
    connect(&pingTimer_, &QTimer::timeout, this, &PingManager::onPingTimer);
}

void PingManager::updateIps(const QVector<PingIpInfo> &ips)
{
    addLog("PingManager::updateIps", "update ips:" + QString::number(ips.count()));

    for (auto it = ips_.begin(); it != ips_.end(); ++it) {
        it.value().existThisIp = false;
    }

    for (const PingIpInfo &ip_info : std::as_const(ips)) {
        auto it = ips_.find(ip_info.ip);
        if (it == ips_.end()) {
            pingStorage_.initPingDataIfNotExists(ip_info.ip);
            PingTime pingTime;
            qint64 iterTime;
            pingStorage_.getPingData(ip_info.ip, pingTime, iterTime);
            ips_[ip_info.ip] = PingIpState(ip_info, iterTime, pingTime == PingTime::PING_FAILED);
        }
        else {
            it.value().existThisIp = true;
        }
    }

    // remove unused ips
    auto it = ips_.begin();
    while (it != ips_.end()) {
        if (!it.value().existThisIp) {
            addLog("PingManager::updateIps", "removed unused ip: " + it.key());
            pingStorage_.removePingNode(it.key());
            it = ips_.erase(it);
        }
        else {
            ++it;
        }
    }

    onPingTimer();
    pingTimer_.start(PING_TIMER_INTERVAL);
}

void PingManager::clearIps()
{
    updateIps(QVector<PingIpInfo>());
}

bool PingManager::isAllNodesHaveCurIteration() const
{
    return pingStorage_.isAllNodesHaveCurIteration();
}

PingTime PingManager::getPing(const QString &ip) const
{
    return pingStorage_.getPing(ip);
}

void PingManager::onPingTimer()
{
    using namespace std::placeholders;
    // We don't attempt to issue a ping request when state is CONNECT_STATE_CONNECTING, as the firewall will block it.
    if (!networkDetectionManager_->isOnline() || connectStateController_->currentState() != CONNECT_STATE_DISCONNECTED)
        return;

    if (ips_.isEmpty())
        return;

    QDateTime curDateTime = QDateTime::currentDateTimeUtc();
    QDateTime nextDateTime = QDateTime::fromMSecsSinceEpoch(pingStorage_.currentIterationTime(), QTimeZone::utc()).addSecs(NEXT_PERIOD_SECS);

    // if the network has changed or ping by time, then re-ping all nodes
    types::NetworkInterface curNetworkInterface;
    networkDetectionManager_->getCurrentNetworkInterface(curNetworkInterface);
    if (pingStorage_.currentIterationTime() == 0 || curDateTime > nextDateTime || curNetworkInterface.networkOrSsid != pingStorage_.currentIterationNetworkOrSsid()) {
        pingStorage_.setCurrentIterationData(curDateTime.toMSecsSinceEpoch(), curNetworkInterface.networkOrSsid);
        for (auto it = ips_.begin(); it != ips_.end(); ++it) {
            it.value().resetState();
        }
        if (curDateTime > nextDateTime)
            addLog("PingManager::onPingTimer", "Re-ping all nodes by time");
        else
            addLog("PingManager::onPingTimer", "Re-ping all nodes by network change");
    }

    for (auto it = ips_.begin(); it != ips_.end(); ++it) {
        PingIpState &pni = it.value();

        if (pni.nowPinging)
            continue;

        // Checking the option ws-use-icmp-pings and force ICMP pings if enabled.
        wsnet::PingType pingType = pni.ipInfo.pingType;
        if (isUseIcmpPings_) {
            pingType = wsnet::PingType::kIcmp;
        }

        if (pni.latestPingFailed) {
            if (pni.nextTimeForFailedPing == 0 || QDateTime::currentMSecsSinceEpoch() >= pni.nextTimeForFailedPing) {
                pni.nowPinging = true;
                addLog("PingManager::onPingTimer", "start ping because latest ping failed: " + it.key());
                WSNet::instance()->pingManager()->ping(pni.ipInfo.ip.toStdString(), pni.ipInfo.hostname.toStdString(), pingType,
                   [this](const std::string &ip, bool isSuccess, std::int32_t timeMs, bool isFromDisconnectedVpnState) {
                       QMetaObject::invokeMethod(this, [this, ip, isSuccess, timeMs, isFromDisconnectedVpnState] { // NOLINT: false positive for memory leak
                           onPingFinished(ip, isSuccess, timeMs, isFromDisconnectedVpnState);
                    });
                });
            }
        } else if (pni.iterationTime != pingStorage_.currentIterationTime()) {
            addLog("PingManager::onPingTimer", QString::fromLatin1("ping new node: %1 (%2 - %3)").arg(pni.ipInfo.ip, pni.ipInfo.city, pni.ipInfo.nick));
            pni.nowPinging = true;
            WSNet::instance()->pingManager()->ping(pni.ipInfo.ip.toStdString(), pni.ipInfo.hostname.toStdString(), pingType,
                                                   [this](const std::string &ip, bool isSuccess, std::int32_t timeMs, bool isFromDisconnectedVpnState) {
                                                       QMetaObject::invokeMethod(this, [this, ip, isSuccess, timeMs, isFromDisconnectedVpnState] { // NOLINT: false positive for memory leak
                                                           onPingFinished(ip, isSuccess, timeMs, isFromDisconnectedVpnState);
                                                       });
                                                   });
        }
    }
}

void PingManager::onPingFinished(const std::string &ip, bool isSuccess, int32_t timeMs, bool isFromDisconnectedVpnState)
{
    Q_UNUSED(isFromDisconnectedVpnState);
    QString ipStr = QString::fromStdString(ip);

    auto itNode = ips_.find(ipStr);
    if (itNode == ips_.end()) {
        return;
    }
    itNode->nowPinging = false;

    // Note: we only issue ping requests in the disconnected state.  However, it is possible we transitioned to the
    // connecting/connected state between the time the ping request was issued to PingHost and when it was executed.
    PingIpState &p = itNode.value();
    if (isSuccess) {
        p.nextTimeForFailedPing = 0;
        p.latestPingFailed = false;
        p.failedPingsInRow = 0;

        // If the ping was executed in the connected state, we'll mark it as never happening and reissue it when
        // we're back in the disconnected state.
        if (isFromDisconnectedVpnState) {
            p.iterationTime = pingStorage_.currentIterationTime();
            pingStorage_.setPing(ipStr, timeMs);
            emit pingInfoChanged(ipStr, timeMs);
            addLog("PingManager::onPingFinished", QString::fromLatin1("ping successful: %1 (%2 - %3) %4ms").arg(p.ipInfo.ip, p.ipInfo.city, p.ipInfo.nick).arg(timeMs));
        }
        else {
            addLog("PingManager::onPingFinished", QString::fromLatin1("discarding ping while connected: %1 (%2 - %3) %4ms").arg(p.ipInfo.ip, p.ipInfo.city, p.ipInfo.nick).arg(timeMs));
        }
    }
    else {
        p.latestPingFailed = true;
        p.failedPingsInRow++;

        if (p.failedPingsInRow >= MAX_FAILED_PING_IN_ROW) {
            p.failedPingsInRow = 0;
            p.nextTimeForFailedPing = QDateTime::currentMSecsSinceEpoch() + 1000 * 60;
            p.curDelayForFailedPing = MIN_DELAY_FOR_FAILED_IN_ROW_PINGS;

            p.iterationTime = pingStorage_.currentIterationTime();
            pingStorage_.setPing(ipStr, PingTime::PING_FAILED);
            emit pingInfoChanged(ipStr, PingTime::PING_FAILED);

            addLog("PingManager::onPingFinished", QString::fromLatin1("ping failed: %1 (%2 - %3)").arg(p.ipInfo.ip, p.ipInfo.city, p.ipInfo.nick));
        }
        else {
            p.curDelayForFailedPing = exponentialBackoff_GetNextDelay(p.curDelayForFailedPing);
            p.nextTimeForFailedPing = QDateTime::currentMSecsSinceEpoch() + 1000 * p.curDelayForFailedPing;
        }

    }
    if (pingStorage_.isAllNodesHaveCurIteration()) {
        addLog("PingManager::onPingFinished", "All nodes have the same iteration time");
    }

    if (isLogPings_ && isAllIpsHaveCurIteration()) {
        addLog("PingManager::onPingFinished", "isAllIpsHaveCurIteration = true");
    }
}

int PingManager::exponentialBackoff_GetNextDelay(int curDelay, float factor, float jitter, float maxDelay)
{
    float res = std::min((float)curDelay * factor, maxDelay);
    return res + Utils::generateDoubleRandom(0, res * jitter);
}

bool PingManager::isAllIpsHaveCurIteration() const
{
    for (const auto &it : std::as_const(ips_))
        if (it.iterationTime != pingStorage_.currentIterationTime())
            return false;

    return true;
}

void PingManager::addLog(const QString &tag, const QString &str)
{
    if (isLogPings_)
        qCDebug(LOG_PING) << tag << "   " << str;
}
