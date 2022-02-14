#include "pingipscontroller.h"
#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "utils/utils.h"
#include <QDebug>
#include "utils/logger.h"

namespace locationsmodel {

PingIpsController::PingIpsController(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager, PingHost *pingHost, const QString &log_filename) : QObject(parent),
    connectStateController_(stateController), networkDetectionManager_(networkDetectionManager),
    pingLog_(log_filename), pingHost_(pingHost)
{
    connect(pingHost_, SIGNAL(pingFinished(bool,int,QString, bool)), SLOT(onPingFinished(bool,int,QString, bool)));
    connect(&pingTimer_, SIGNAL(timeout()), SLOT(onPingTimer()));

    int pingHour = Utils::generateIntegerRandom(0, 23);
    int pingMinute = Utils::generateIntegerRandom(0, 59);
    int pingSecond = Utils::generateIntegerRandom(0, 59);

    isNeedPingForNextDisconnectState_ = false;
    dtNextPingTime_ = QDateTime::currentDateTime();

    if (dtNextPingTime_.time().hour() < pingHour)
    {
        dtNextPingTime_.setTime(QTime(pingHour, pingMinute, pingSecond));
    }
    else
    {
        dtNextPingTime_.setTime(QTime(pingHour, pingMinute, pingSecond));
        dtNextPingTime_ = dtNextPingTime_.addDays(1);
    }
    //dtNextPingTime_ = dtNextPingTime_.addSecs(20);      // for testing

    pingLog_.addLog("PingIpsController::PingIpsController","set next ping time: " + dtNextPingTime_.toString("ddMMyyyy HH:mm:ss"));
}

void PingIpsController::updateIps(const QVector<PingIpInfo> &ips)
{
    pingLog_.addLog("PingIpsController::updateIps", "update ips");
    for (auto it = ips_.begin(); it != ips_.end(); ++it)
    {
        it.value().existThisIp = false;
    }

    for (const PingIpInfo &ip_info : ips)
    {
        auto it = ips_.find(ip_info.ip_);
        if (it == ips_.end())
        {
            PingNodeInfo pni;
            pni.isExistPingAttempt = false;
            pni.existThisIp = true;
            pni.latestPingFailed_ = false;
            pni.bNowPinging_ = false;
            pni.failedPingsInRow = 0;
            pni.latestPingFromDisconnectedState_ = false;
            pni.nextTimeForFailedPing_ = 0;
            pni.pingType = ip_info.pingType_;
            ips_[ip_info.ip_] = pni;
        }
        else
        {
            it.value().existThisIp = true;
        }
    }

    // remove unused ips
    auto it = ips_.begin();
    while (it != ips_.end())
    {
        if (!it.value().existThisIp)
        {
            pingLog_.addLog("PingIpsController::updateIps", "removed unused ip: " + it.key());
            it = ips_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    failedPingLogController_.clear();

    onPingTimer();
    pingTimer_.start(PING_TIMER_INTERVAL);
}

void PingIpsController::onPingTimer()
{
    if (!networkDetectionManager_->isOnline())
    {
        return;
    }

    bool bNeedPingByTime = QDateTime::currentDateTime() > dtNextPingTime_;
    if (bNeedPingByTime)
    {
        dtNextPingTime_ = dtNextPingTime_.addDays(1);
        //dtNextPingTime_ = dtNextPingTime_.addSecs(20);      // for testing
        qCDebug(LOG_BASIC) << "Ping all nodes by time";
        pingLog_.addLog("PingIpsController::onPingTimer", "it's ping time, set next ping time to:" + dtNextPingTime_.toString("ddMMyyyy HH:mm:ss"));
        Q_EMIT needIncrementPingIteration();
    }

    CONNECT_STATE curConnectState = connectStateController_->currentState();

    if (bNeedPingByTime && curConnectState != CONNECT_STATE_DISCONNECTED)
    {
        isNeedPingForNextDisconnectState_ = true;
    }

    for (QHash<QString, PingNodeInfo>::iterator it = ips_.begin(); it != ips_.end(); ++it)
    {
        PingNodeInfo pni = it.value();

        if (it.value().bNowPinging_)
        {
            continue;
        }

        if ((bNeedPingByTime || isNeedPingForNextDisconnectState_) && curConnectState == CONNECT_STATE_DISCONNECTED)
        {
            pingLog_.addLog("PingNodesController::onPingTimer", "start ping by time: " + it.key());
            it.value().bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), it.value().pingType);
        }
        else if (!pni.isExistPingAttempt)
        {
            pingLog_.addLog("PingNodesController::onPingTimer", "start ping the new node: " + it.key());
            it.value().bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), it.value().pingType);
        }
        else if (pni.latestPingFailed_)
        {
            if (pni.nextTimeForFailedPing_ == 0 || QDateTime::currentMSecsSinceEpoch() >= pni.nextTimeForFailedPing_)
            {
                //pingLog_.addLog("PingNodesController::onPingTimer", "start ping because latest ping failed: " + it.key());
                it.value().bNowPinging_ = true;
                pingHost_->addHostForPing(it.key(), it.value().pingType);
            }
        }
        else if (!pni.latestPingFromDisconnectedState_ && curConnectState == CONNECT_STATE_DISCONNECTED)
        {
            pingLog_.addLog("PingNodesController::onPingTimer", "start ping from disconnected state, because latest ping was in connected state: " + it.key());
            it.value().bNowPinging_ = true;
            pingHost_->addHostForPing(it.key(), it.value().pingType);
        }
    }

    if (isNeedPingForNextDisconnectState_ && curConnectState == CONNECT_STATE_DISCONNECTED)
    {
        isNeedPingForNextDisconnectState_ = false;
    }
}

void PingIpsController::onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState)
{
    auto itNode = ips_.find(ip);
    if (itNode != ips_.end())
    {
        itNode.value().bNowPinging_ = false;
        if (bSuccess)
        {
            if (isFromDisconnectedState)
            {
                pingLog_.addLog("PingIpsController::onPingFinished", "ping successfully from disconnected state: " + ip + " " + QString::number(timems) + "ms");
                itNode.value().isExistPingAttempt = true;
                itNode.value().latestPingFailed_ = false;
                itNode.value().latestPingFromDisconnectedState_ = true;
                itNode.value().failedPingsInRow = 0;
                Q_EMIT pingInfoChanged(ip, timems, true);
            }
            else
            {
                pingLog_.addLog("PingIpsController::onPingFinished", "ping successfully from connected state: " + ip + " " + QString::number(timems) + "ms");
                itNode.value().isExistPingAttempt = true;
                itNode.value().latestPingFailed_ = false;
                itNode.value().latestPingFromDisconnectedState_ = false;
                itNode.value().failedPingsInRow = 0;
                Q_EMIT pingInfoChanged(ip, timems, false);
            }
        }
        else
        {
            itNode.value().isExistPingAttempt = true;
            itNode.value().latestPingFailed_ = true;
            itNode.value().failedPingsInRow++;

            if (itNode.value().failedPingsInRow >= MAX_FAILED_PING_IN_ROW)
            {
                //pingLog_.addLog("PingIpsController::onPingFinished", "ping failed 3 times at row: " + ip);
                itNode.value().failedPingsInRow = 0;
                // next ping attempt in 1 mins
                itNode.value().nextTimeForFailedPing_ = QDateTime::currentMSecsSinceEpoch() + 1000 * 60;
                Q_EMIT pingInfoChanged(ip, PingTime::PING_FAILED, isFromDisconnectedState);
                if (failedPingLogController_.logFailedIPs(ip))
                {
                    pingLog_.addLog("PingIpsController::onPingFinished", "ping failed: " + ip);
                }
            }
            else
            {
                itNode.value().nextTimeForFailedPing_ = 0;
                //pingLog_.addLog("PingIpsController::onPingFinished", "ping failed: " + ip);
            }
        }
    }
}

} //namespace locationsmodel
