#include "pingmanager.h"

#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "types/pingtime.h"

PingManager::PingManager(QObject *parent, IConnectStateController *stateController,
                                     INetworkDetectionManager *networkDetectionManager, PingMultipleHosts *pingHosts, const QString &storageSettingName,
                                     const QString &log_filename) : QObject(parent),
    connectStateController_(stateController), networkDetectionManager_(networkDetectionManager), pingStorage_(storageSettingName),
    pingLog_(log_filename), pingHosts_(pingHosts)
{
    connect(pingHosts_, &PingMultipleHosts::pingFinished, this, &PingManager::onPingFinished);
    connect(&pingTimer_, &QTimer::timeout, this, &PingManager::onPingTimer);
}

void PingManager::updateIps(const QVector<PingIpInfo> &ips)
{
    pingLog_.addLog("PingIpsController::updateIps", "update ips:" + QString::number(ips.count()));

    for (auto it = ips_.begin(); it != ips_.end(); ++it) {
        it.value().existThisIp = false;
    }

    for (const PingIpInfo &ip_info : qAsConst(ips)) {
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
            pingLog_.addLog("PingIpsController::updateIps", "removed unused ip: " + it.key());
            pingStorage_.removePingNode(it.key());
            it = ips_.erase(it);
        }
        else {
            ++it;
        }
    }

    failedPingLogController_.clear();

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
    // We don't attempt to issue a ping request when state is CONNECT_STATE_CONNECTING, as the firewall will block it.
    if (!networkDetectionManager_->isOnline() || connectStateController_->currentState() != CONNECT_STATE_DISCONNECTED)
        return;

    if (ips_.isEmpty())
        return;

    QDateTime curDateTime = QDateTime::currentDateTimeUtc();
    QDateTime nextDateTime = QDateTime::fromMSecsSinceEpoch(pingStorage_.currentIterationTime(), Qt::UTC).addSecs(NEXT_PERIOD_SECS);

    // if the network has changed or ping by time, then re-ping all nodes
    types::NetworkInterface curNetworkInterface;
    networkDetectionManager_->getCurrentNetworkInterface(curNetworkInterface);
    if (pingStorage_.currentIterationTime() == 0 || curDateTime > nextDateTime || curNetworkInterface.networkOrSsid != pingStorage_.currentIterationNetworkOrSsid()) {
        pingStorage_.setCurrentIterationData(curDateTime.toMSecsSinceEpoch(), curNetworkInterface.networkOrSsid);
        for (auto it = ips_.begin(); it != ips_.end(); ++it) {
            it.value().resetState();
        }
        if (curDateTime > nextDateTime)
            pingLog_.addLog("PingIpsController::onPingTimer", "Re-ping all nodes by time");
        else
            pingLog_.addLog("PingIpsController::onPingTimer", "Re-ping all nodes by network change");
    }

    for (auto it = ips_.begin(); it != ips_.end(); ++it) {
        PingIpState &pni = it.value();
        if (pni.iterationTime != pingStorage_.currentIterationTime()) {
            pingLog_.addLog("PingNodesController::onPingTimer", QString::fromLatin1("ping new node: %1 (%2 - %3)").arg(pni.ipInfo.ip, pni.ipInfo.city, pni.ipInfo.nick));
            pingHosts_->addHostForPing(pni.ipInfo.ip, pni.ipInfo.hostname, pni.ipInfo.pingType);
        } else if (pni.latestPingFailed) {
            if (pni.nextTimeForFailedPing == 0 || QDateTime::currentMSecsSinceEpoch() >= pni.nextTimeForFailedPing) {
                pingLog_.addLog("PingNodesController::onPingTimer", "start ping because latest ping failed: " + it.key());
                pingHosts_->addHostForPing(pni.ipInfo.ip, pni.ipInfo.hostname, pni.ipInfo.pingType);
            }
        }
    }
}

void PingManager::onPingFinished(bool success, int timems, const QString &ip, bool isFromDisconnectedState)
{
    auto itNode = ips_.find(ip);
    if (itNode == ips_.end()) {
        return;
    }

    // Note: we only issue ping requests in the disconnected state.  However, it is possible we transitioned to the
    // connecting/connected state between the time the ping request was issued to PingHost and when it was executed.
    PingIpState &p = itNode.value();
    if (success) {
        p.nextTimeForFailedPing = 0;
        p.latestPingFailed = false;
        p.failedPingsInRow = 0;

        // If the ping was executed in the connected state, we'll mark it as never happening and reissue it when
        // we're back in the disconnected state.
        if (isFromDisconnectedState) {
            p.iterationTime = pingStorage_.currentIterationTime();
            pingStorage_.setPing(ip, timems);
            emit pingInfoChanged(ip, timems);
            pingLog_.addLog("PingIpsController::onPingFinished", QString::fromLatin1("ping successful: %1 (%2 - %3) %4ms").arg(p.ipInfo.ip, p.ipInfo.city, p.ipInfo.nick).arg(timems));
        }
        else {
            pingLog_.addLog("PingIpsController::onPingFinished", QString::fromLatin1("discarding ping while connected: %1 (%2 - %3) %4ms").arg(p.ipInfo.ip, p.ipInfo.city, p.ipInfo.nick).arg(timems));
        }
    }
    else {
        p.latestPingFailed = true;
        p.failedPingsInRow++;

        if (p.failedPingsInRow >= MAX_FAILED_PING_IN_ROW) {
            p.failedPingsInRow = 0;
            p.nextTimeForFailedPing = QDateTime::currentMSecsSinceEpoch() + 1000 * 60;

            if (isFromDisconnectedState) {
                p.iterationTime = pingStorage_.currentIterationTime();
                pingStorage_.setPing(ip, PingTime::PING_FAILED);
                emit pingInfoChanged(ip, PingTime::PING_FAILED);
            }

            if (failedPingLogController_.logFailedIPs(ip)) {
                pingLog_.addLog("PingIpsController::onPingFinished", QString::fromLatin1("ping failed: %1 (%2 - %3)").arg(p.ipInfo.ip, p.ipInfo.city, p.ipInfo.nick));
            }
        }
        else {
            p.nextTimeForFailedPing = 0;
        }
    }
    if (pingStorage_.isAllNodesHaveCurIteration()) {
        pingLog_.addLog("PingIpsController::onPingFinished", "All nodes have the same iteration time");
    }
}
