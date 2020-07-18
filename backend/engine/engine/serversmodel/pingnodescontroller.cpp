#include "pingnodescontroller.h"
#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "utils/utils.h"
#include "failedpinglogcontroller.h"
#include <QDebug>
#include "utils/logger.h"

const int typeIdVectorPingNodeAndType = qRegisterMetaType< QVector<PingNodesController::PingNodeAndType> >("QVector<PingNodesController::PingNodeAndType>");

PingNodesController::PingNodesController(QObject *parent, IConnectStateController *stateController, NodesSpeedStore *nodesSpeedStore, INetworkStateManager *networkStateManager, PingLog &pingLog) : QObject(parent),
    connectStateController_(stateController), nodesSpeedStore_(nodesSpeedStore), networkStateManager_(networkStateManager), pingLog_(pingLog),
    pingHost_(this, stateController)
{
    iteration_ = 1;

    connect(&pingHost_, SIGNAL(pingFinished(bool,int,QString, bool)), SLOT(onPingFinished(bool,int,QString, bool)));
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
    //dtNextPingTime_ = dtNextPingTime_.addSecs(30);

    pingLog_.addLog("PingNodesController::PingNodesController","set next ping time: " + dtNextPingTime_.toString("ddMMyyyy HH:mm:ss"));
}

void PingNodesController::setProxySettings(const ProxySettings &proxySettings)
{
    pingHost_.setProxySettings(proxySettings);
}

void PingNodesController::disableProxy()
{
    pingHost_.disableProxy();
}

void PingNodesController::enableProxy()
{
    pingHost_.enableProxy();
}

void PingNodesController::updateNodes(QVector<PingNodesController::PingNodeAndType> nodes)
{
    pingLog_.addLog("PingNodesController::updateNodes", "update nodes");
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
    {
        it.value().existThisIp = false;
    }

    Q_FOREACH(const PingNodeAndType &nd, nodes)
    {
        auto it = nodes_.find(nd.ip);
        if (it == nodes_.end())
        {
            PingNodeInfo pni;
            pni.isExistPingAttempt = false;
            pni.existThisIp = true;
            pni.latestPingFailed_ = false;
            pni.bNowPinging_ = false;
            pni.failedPingsInRow = 0;
            pni.pingType = nd.pingType;
            nodes_[nd.ip] = pni;
        }
        else
        {
            it.value().existThisIp = true;
        }
    }

    // remove unused ips
    QHash<QString, PingNodeInfo>::iterator it = nodes_.begin();
    while (it != nodes_.end())
    {
        if (!it.value().existThisIp)
        {
            pingLog_.addLog("PingNodesController::updateNodes", "removed unused ip: " + it.key());
            it = nodes_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    FailedPingLogController::instance().clear();

    onPingTimer();
    pingTimer_.start(PING_TIMER_INTERVAL);
}

void PingNodesController::onPingTimer()
{
    if (!networkStateManager_->isOnline())
    {
        return;
    }

    bool bNeedPingByTime = QDateTime::currentDateTime() > dtNextPingTime_;
    if (bNeedPingByTime)
    {
        dtNextPingTime_ = dtNextPingTime_.addDays(1);
        qCDebug(LOG_BASIC) << "Ping all nodes by time";
        pingLog_.addLog("PingNodesController::onPingTimer", "it's ping time, set next ping time to:" + dtNextPingTime_.toString("ddMMyyyy HH:mm:ss"));
        iteration_++;
    }

    CONNECT_STATE curConnectState = connectStateController_->currentState();

    if (bNeedPingByTime && curConnectState != CONNECT_STATE_DISCONNECTED)
    {
        isNeedPingForNextDisconnectState_ = true;
    }

    for (QHash<QString, PingNodeInfo>::iterator it = nodes_.begin(); it != nodes_.end(); ++it)
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
            pingHost_.addHostForPing(it.key(), it.value().pingType);
        }
        else if (!pni.isExistPingAttempt)
        {
            pingLog_.addLog("PingNodesController::onPingTimer", "start ping the new node: " + it.key());
            it.value().bNowPinging_ = true;
            pingHost_.addHostForPing(it.key(), it.value().pingType);
        }
        else if (pni.latestPingFailed_)
        {
            if (pni.nextTimeForFailedPing_ == 0 || QDateTime::currentMSecsSinceEpoch() >= pni.nextTimeForFailedPing_)
            {
                pingLog_.addLog("PingNodesController::onPingTimer", "start ping because latest ping failed: " + it.key());
                it.value().bNowPinging_ = true;
                pingHost_.addHostForPing(it.key(), it.value().pingType);
            }
        }
        else if (!pni.latestPingFromDisconnectedState_ && curConnectState == CONNECT_STATE_DISCONNECTED)
        {
            pingLog_.addLog("PingNodesController::onPingTimer", "start ping from disconnected state, because latest ping was in connected state: " + it.key());
            it.value().bNowPinging_ = true;
            emit pingStartedInDisconnectedState(it.key());
            pingHost_.addHostForPing(it.key(), it.value().pingType);
        }
    }

    if (isNeedPingForNextDisconnectState_ && curConnectState == CONNECT_STATE_DISCONNECTED)
    {
        isNeedPingForNextDisconnectState_ = false;
    }
}

void PingNodesController::onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState)
{
    auto itNode = nodes_.find(ip);
    if (itNode != nodes_.end())
    {
        itNode.value().bNowPinging_ = false;
        if (bSuccess)
        {
            if (isFromDisconnectedState)
            {
                pingLog_.addLog("PingNodesController::onPingFinished", "ping successfully from disconnected state: " + ip + " " + QString::number(timems) + "ms");
                itNode.value().isExistPingAttempt = true;
                itNode.value().latestPingFailed_ = false;
                itNode.value().latestPingFromDisconnectedState_ = true;
                itNode.value().failedPingsInRow = 0;
                nodesSpeedStore_->setNodeSpeed(ip, timems, iteration_);
                emit pingInfoChanged(ip, timems, iteration_);
            }
            else
            {
                pingLog_.addLog("PingNodesController::onPingFinished", "ping successfully from connected state: " + ip + " " + QString::number(timems) + "ms");
                itNode.value().isExistPingAttempt = true;
                itNode.value().latestPingFailed_ = false;
                itNode.value().latestPingFromDisconnectedState_ = false;
                itNode.value().failedPingsInRow = 0;
                emit pingInfoChanged(ip, timems, iteration_);
            }
        }
        else
        {
            itNode.value().isExistPingAttempt = true;
            itNode.value().latestPingFailed_ = true;
            itNode.value().failedPingsInRow++;

            nodesSpeedStore_->setNodeSpeed(ip, -1, iteration_);

            if (itNode.value().failedPingsInRow >= MAX_FAILED_PING_IN_ROW)
            {
                pingLog_.addLog("PingNodesController::onPingFinished", "ping failed 3 times at row: " + ip);
                itNode.value().failedPingsInRow = 0;
                // next ping attempt in 1 mins
                itNode.value().nextTimeForFailedPing_ = QDateTime::currentMSecsSinceEpoch() + 1000 * 60;
                emit pingInfoChanged(ip, -1, iteration_);
                FailedPingLogController::instance().logFailedIPs(QStringList() << ip);
            }
            else
            {
                itNode.value().nextTimeForFailedPing_ = 0;
                pingLog_.addLog("PingNodesController::onPingFinished", "ping failed: " + ip);
            }
        }
    }
}
