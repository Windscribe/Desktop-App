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

    isNeedPingForNextDisconnectState_ = false;
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
            pni.latestPingFromDisconnectedState_ = false;
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
    if (!networkDetectionManager_->isOnline()) {
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

    CONNECT_STATE curConnectState = connectStateController_->currentState();

    if (bNeedPingByTime && curConnectState != CONNECT_STATE_DISCONNECTED) {
        isNeedPingForNextDisconnectState_ = true;
    }

    for (QHash<QString, PingNodeInfo>::iterator it = ips_.begin(); it != ips_.end(); ++it) {
        PingNodeInfo pni = it.value();

        if (it.value().bNowPinging_) {
            continue;
        }

        if ((bNeedPingByTime || isNeedPingForNextDisconnectState_) && curConnectState == CONNECT_STATE_DISCONNECTED) {
            pingLog_.addLog("PingNodesController::onPingTimer", tr("start ping by time for: %1 (%2 - %3)").arg(it.key(), pni.city_, pni.nick_));
            it.value().bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), it.value().pingType);
        }
        else if (!pni.isExistPingAttempt) {
            pingLog_.addLog("PingNodesController::onPingTimer", tr("ping new node: %1 (%2 - %3)").arg(it.key(), pni.city_, pni.nick_));
            it.value().bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), it.value().pingType);
        }
        else if (pni.latestPingFailed_) {
            if (pni.nextTimeForFailedPing_ == 0 || QDateTime::currentMSecsSinceEpoch() >= pni.nextTimeForFailedPing_) {
                //pingLog_.addLog("PingNodesController::onPingTimer", "start ping because latest ping failed: " + it.key());
                it.value().bNowPinging_ = true;
                pingHost_->addHostForPing(it.key(), it.value().pingType);
            }
        }
        else if (!pni.latestPingFromDisconnectedState_ && curConnectState == CONNECT_STATE_DISCONNECTED) {
            pingLog_.addLog("PingNodesController::onPingTimer", tr("start ping from disconnected state, because latest ping was in connected state: %1 (%2 - %3)").arg(it.key(), pni.city_, pni.nick_));
            it.value().bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), it.value().pingType);
        }
    }

    if (isNeedPingForNextDisconnectState_ && curConnectState == CONNECT_STATE_DISCONNECTED) {
        isNeedPingForNextDisconnectState_ = false;
    }
}

void PingIpsController::onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState)
{
    auto itNode = ips_.find(ip);
    if (itNode != ips_.end()) {
        itNode.value().bNowPinging_ = false;

        if (bSuccess) {
            itNode.value().isExistPingAttempt = true;
            itNode.value().latestPingFailed_ = false;
            itNode.value().latestPingFromDisconnectedState_ = isFromDisconnectedState;
            itNode.value().failedPingsInRow = 0;

            pingLog_.addLog("PingIpsController::onPingFinished",
                tr("ping successfully from %1 state: %2 (%3 - %4) %5ms").arg(isFromDisconnectedState ? tr("disconnected") : tr("connected"), ip, itNode->city_, itNode->nick_).arg(timems));

            Q_EMIT pingInfoChanged(ip, timems, isFromDisconnectedState);
        }
        else {
            itNode.value().isExistPingAttempt = true;
            itNode.value().latestPingFailed_ = true;
            itNode.value().failedPingsInRow++;

            if (itNode.value().failedPingsInRow >= MAX_FAILED_PING_IN_ROW) {
                //pingLog_.addLog("PingIpsController::onPingFinished", "ping failed 3 times at row: " + ip);
                itNode.value().failedPingsInRow = 0;
                // next ping attempt in 1 mins
                itNode.value().nextTimeForFailedPing_ = QDateTime::currentMSecsSinceEpoch() + 1000 * 60;
                Q_EMIT pingInfoChanged(ip, PingTime::PING_FAILED, isFromDisconnectedState);
                if (failedPingLogController_.logFailedIPs(ip)) {
                    pingLog_.addLog("PingIpsController::onPingFinished", tr("ping failed: %1 (%2 - %3)").arg(ip, itNode->city_, itNode->nick_));
                }
            }
            else {
                itNode.value().nextTimeForFailedPing_ = 0;
                //pingLog_.addLog("PingIpsController::onPingFinished", "ping failed: " + ip);
            }
        }
    }
}

} //namespace locationsmodel
