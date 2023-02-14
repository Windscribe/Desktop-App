#include "pingipscontroller.h"

#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "utils/logger.h"
#include "types/pingtime.h"
#include "utils/utils.h"

namespace locationsmodel {

PingIpsController::PingIpsController(QObject *parent, IConnectStateController *stateController,
                                     INetworkDetectionManager *networkDetectionManager, PingHost *pingHost,
                                     const QString &log_filename) : QObject(parent),
    connectStateController_(stateController), networkDetectionManager_(networkDetectionManager),
    pingLog_(log_filename), pingHost_(pingHost)
{
    connect(pingHost_, &PingHost::pingFinished, this, &PingIpsController::onPingFinished);
    connect(&pingTimer_, &QTimer::timeout, this, &PingIpsController::onPingTimer);

    int pingHour = Utils::generateIntegerRandom(0, 23);
    int pingMinute = Utils::generateIntegerRandom(0, 59);
    int pingSecond = Utils::generateIntegerRandom(0, 59);

    dtNextPingTime_ = QDateTime::currentDateTime();

    if (dtNextPingTime_.time().hour() < pingHour) {
        dtNextPingTime_.setTime(QTime(pingHour, pingMinute, pingSecond));
    }
    else {
        dtNextPingTime_.setTime(QTime(pingHour, pingMinute, pingSecond));
        dtNextPingTime_ = dtNextPingTime_.addDays(1);
    }
    //dtNextPingTime_ = dtNextPingTime_.addSecs(20);      // for testing

    pingLog_.addLog("PingIpsController::PingIpsController","set next ping time: " + dtNextPingTime_.toString("ddMMyyyy HH:mm:ss"));
}

void PingIpsController::updateIps(const QVector<PingIpInfo> &ips)
{
    pingLog_.addLog("PingIpsController::updateIps", "update ips");
    for (auto it = ips_.begin(); it != ips_.end(); ++it) {
        it.value().existThisIp = false;
    }

    for (const PingIpInfo &ip_info : ips) {
        auto it = ips_.find(ip_info.ip_);
        if (it == ips_.end()) {
            PingNodeInfo pni;
            pni.isExistPingAttempt = false;
            pni.existThisIp = true;
            pni.latestPingFailed_ = false;
            pni.bNowPinging_ = false;
            pni.failedPingsInRow = 0;
            pni.nextTimeForFailedPing_ = 0;
            pni.pingType = ip_info.pingType_;
            pni.city_ = ip_info.city_;
            pni.nick_ = ip_info.nick_;
            ips_[ip_info.ip_] = pni;
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

void PingIpsController::onPingTimer()
{
    // We don't attempt to issue a ping request when state is CONNECT_STATE_CONNECTING, as the firewall will block it.
    if (!networkDetectionManager_->isOnline() || connectStateController_->currentState() != CONNECT_STATE_DISCONNECTED) {
        return;
    }

    bool bNeedPingByTime = QDateTime::currentDateTime() > dtNextPingTime_;
    if (bNeedPingByTime) {
        dtNextPingTime_ = dtNextPingTime_.addDays(1);
        //dtNextPingTime_ = dtNextPingTime_.addSecs(20);      // for testing
        qCDebug(LOG_BASIC) << "Ping all nodes by time";
        pingLog_.addLog("PingIpsController::onPingTimer", "it's ping time, set next ping time to:" + dtNextPingTime_.toString("ddMMyyyy HH:mm:ss"));
        Q_EMIT needIncrementPingIteration();
    }

    for (QHash<QString, PingNodeInfo>::iterator it = ips_.begin(); it != ips_.end(); ++it) {
        PingNodeInfo &pni = it.value();

        if (pni.bNowPinging_) {
            continue;
        }

        if (bNeedPingByTime) {
            pingLog_.addLog("PingNodesController::onPingTimer", tr("start ping by time for: %1 (%2 - %3)").arg(it.key(), pni.city_, pni.nick_));
            pni.bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), pni.pingType);
        }
        else if (!pni.isExistPingAttempt) {
            pingLog_.addLog("PingNodesController::onPingTimer", tr("ping new node: %1 (%2 - %3)").arg(it.key(), pni.city_, pni.nick_));
            pni.bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), pni.pingType);
        }
        else if (pni.latestPingFailed_) {
            if (pni.nextTimeForFailedPing_ == 0 || QDateTime::currentMSecsSinceEpoch() >= pni.nextTimeForFailedPing_) {
                //pingLog_.addLog("PingNodesController::onPingTimer", "start ping because latest ping failed: " + it.key());
                pni.bNowPinging_ = true;
                pingHost_->addHostForPing(it.key(), pni.pingType);
            }
        }
    }
}

void PingIpsController::onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState)
{
    auto itNode = ips_.find(ip);
    if (itNode == ips_.end()) {
        return;
    }

    // Note: we only issue ping requests in the disconnected state.  However, it is possible we transitioned to the
    // connecting/connected state between the time the ping request was issued to PingHost and when it was executed.

    PingNodeInfo &pni = itNode.value();
    pni.bNowPinging_ = false;

    if (bSuccess) {
        // If the ping was executed in the connected state, we'll mark it as never happening and reissue it when
        // we're back in the disconnected state.
        pni.isExistPingAttempt = isFromDisconnectedState;
        pni.latestPingFailed_ = false;
        pni.failedPingsInRow = 0;

        pingLog_.addLog("PingIpsController::onPingFinished",
            tr("ping successfully from %1 state: %2 (%3 - %4) %5ms").arg(isFromDisconnectedState ? tr("disconnected") : tr("connected"), ip, itNode->city_, itNode->nick_).arg(timems));

        if (isFromDisconnectedState) {
            Q_EMIT pingInfoChanged(ip, timems, true);
        }
    }
    else {
        pni.isExistPingAttempt = true;
        pni.latestPingFailed_ = true;
        pni.failedPingsInRow++;

        if (pni.failedPingsInRow >= MAX_FAILED_PING_IN_ROW) {
            //pingLog_.addLog("PingIpsController::onPingFinished", "ping failed 3 times in a row: " + ip);
            pni.failedPingsInRow = 0;
            pni.nextTimeForFailedPing_ = QDateTime::currentMSecsSinceEpoch() + 1000 * 60;

            if (isFromDisconnectedState) {
                Q_EMIT pingInfoChanged(ip, PingTime::PING_FAILED, true);
            }

            if (failedPingLogController_.logFailedIPs(ip)) {
                pingLog_.addLog("PingIpsController::onPingFinished", tr("ping failed: %1 (%2 - %3)").arg(ip, itNode->city_, itNode->nick_));
            }
        }
        else {
            pni.nextTimeForFailedPing_ = 0;
            //pingLog_.addLog("PingIpsController::onPingFinished", "ping failed: " + ip);
        }
    }
}

} //namespace locationsmodel
