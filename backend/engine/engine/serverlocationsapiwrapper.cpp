#include "serverlocationsapiwrapper.h"
#include "Utils/logger.h"
#include <limits>

ServerLocationsApiWrapper::ServerLocationsApiWrapper(QObject *parent, NodesSpeedStore *nodesSpeedStore, ServerAPI *serverAPI) : QObject(parent),
    nodesSpeedStore_(nodesSpeedStore), serverAPI_(serverAPI)
{
    connect(serverAPI_, SIGNAL(serverLocationsAnswer(SERVER_API_RET_CODE,QVector<QSharedPointer<ServerLocation> >,QStringList, uint)),
                            SLOT(onServerLocationsAnswer(SERVER_API_RET_CODE,QVector<QSharedPointer<ServerLocation> >,QStringList, uint)), Qt::QueuedConnection);
    connect(nodesSpeedStore_, SIGNAL(pingIterationChanged()), SLOT(onPingIterationChanged()), Qt::QueuedConnection);

    bestLocation_.readFromSettings();

    pingHost_ = new PingHost(this, NULL);
    connect(pingHost_, SIGNAL(pingFinished(bool,int,QString,bool)), SLOT(onPingFinished(bool,int,QString,bool)));
}

ServerLocationsApiWrapper::~ServerLocationsApiWrapper()
{
    pingHost_->clearPings();
    bestLocation_.writeToSettings();
}

void ServerLocationsApiWrapper::serverLocations(const QString &authHash, const QString &language, uint userRole, bool isNeedCheckRequestsEnabled,
                                                const QString &revision, bool isPro, ProtocolType protocol, const QStringList &alcList)
{
    serverAPI_->serverLocations(authHash, language, userRole, isNeedCheckRequestsEnabled, revision, isPro, protocol, alcList);
}

void ServerLocationsApiWrapper::onServerLocationsAnswer(SERVER_API_RET_CODE retCode, QVector<QSharedPointer<ServerLocation> > serverLocations, QStringList forceDisconnectNodes, uint userRole)
{
    if (retCode != SERVER_RETURN_SUCCESS)
    {
        emit serverLocationsAnswer(retCode, serverLocations, forceDisconnectNodes, userRole);
        return;
    }

    // need update firewall exceptions for ping
    emit updateFirewallIpsForLocations(serverLocations);

    pingHost_->clearPings();
    serverLocations_.clear();
    QStringList allIps;
    Q_FOREACH(QSharedPointer<ServerLocation> sl, serverLocations)
    {
        QVector<ServerNode> &nodes = sl->getNodes();
        Q_FOREACH(const ServerNode &sn, nodes)
        {
            allIps << sn.getIpForPing();
        }

        QSharedPointer<ServerLocation> copySl(new ServerLocation);
        *copySl = *sl;
        serverLocations_.push_back(copySl);
    }

    forceDisconnectNodes_ = forceDisconnectNodes;
    userRole_ = userRole;

    nodesSpeedStore_->updateNodes(allIps);

    QSet<QString> nodesWithoutPingData = nodesSpeedStore_->getNodesWithoutPingData(allIps);

    int bestLocationInd;
    int bestLocationSelNodeInd;
    int bestLocationTimeMs;
    if (nodesWithoutPingData.isEmpty() && detectBestLocation(serverLocations, bestLocationInd, bestLocationSelNodeInd, bestLocationTimeMs))
    {
        if (isNeedChangeBestLocation(bestLocationSelNodeInd, bestLocationTimeMs))
        {
            bestLocation_.set(serverLocations[bestLocationInd]->getId(), serverLocations[bestLocationInd]->getNodes()[bestLocationSelNodeInd].getHostname(), bestLocationTimeMs);
            outputBestLocationToLog(bestLocationInd, bestLocationSelNodeInd, bestLocationTimeMs);
        }

        QSharedPointer<ServerLocation> bestServerLocation = createBestLocation();
        if (!bestServerLocation.isNull())
        {
            serverLocations.insert(0, bestServerLocation);
        }
        emit serverLocationsAnswer(retCode, serverLocations, forceDisconnectNodes, userRole);
    }
    else if (nodesWithoutPingData.isEmpty())
    {
        qCDebug(LOG_BEST_LOCATION()) << "Can't detect best location, all pings failed";
        qCDebug(LOG_BEST_LOCATION()) << "Choose the random best location";
        selectRandomBestLocation();
    }
    else
    {
        qCDebug(LOG_BEST_LOCATION()) << "Can't detect best location from saved data, ping nodes";
        // if we can't detect BestLocation immediatelly, then make pings for nodes without ping data
        // after pings finished, detect best location and return servers locations with best location
        pings_.clear();
        Q_FOREACH(const QString &ip, nodesWithoutPingData)
        {
            pings_[ip] = PingTime::NO_PING_INFO;
            pingHost_->addHostForPing(ip, PingHost::PING_TCP);
        }
    }
}

void ServerLocationsApiWrapper::onPingIterationChanged()
{
    qCDebug(LOG_BEST_LOCATION()) << "Ping iteration finished";

    if (pings_.count() > 0)
    {
        qCDebug(LOG_BEST_LOCATION()) << "Ping iteration, skip handle";
        return;
    }

    Q_ASSERT(bestLocation_.isValid());

    int bestLocationInd;
    int bestLocationSelNodeInd;
    int bestLocationTimeMs;
    if (detectBestLocation(serverLocations_, bestLocationInd, bestLocationSelNodeInd, bestLocationTimeMs))
    {
        // if best location data changed, then emit signal updatedBestLocation
        if (isNeedChangeBestLocation(bestLocationSelNodeInd, bestLocationTimeMs))
        {
            bestLocation_.set(serverLocations_[bestLocationInd]->getId(), serverLocations_[bestLocationInd]->getNodes()[bestLocationSelNodeInd].getHostname(), bestLocationTimeMs);
            outputBestLocationToLog(bestLocationInd, bestLocationSelNodeInd, bestLocationTimeMs);

            QVector< QSharedPointer<ServerLocation> > serverLocationsWithBestLocation;

            QSharedPointer<ServerLocation> bestServerLocation = createBestLocation();
            if (!bestServerLocation.isNull())
            {
                serverLocationsWithBestLocation << bestServerLocation;
            }

            for (auto it = serverLocations_.begin(); it != serverLocations_.end(); it++)
            {
                 QSharedPointer<ServerLocation> slCopy(new ServerLocation);
                 *slCopy = **it;
                 serverLocationsWithBestLocation << slCopy;
            }
            emit updatedBestLocation(serverLocationsWithBestLocation);
        }
    }
}

void ServerLocationsApiWrapper::onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState)
{
    Q_UNUSED(isFromDisconnectedState);

    auto it = pings_.find(ip);
    if (it != pings_.end())
    {
        if (bSuccess)
        {
            *it = timems;
            nodesSpeedStore_->setNodeSpeed(ip, timems, 0);
        }
        else
        {
            nodesSpeedStore_->setNodeSpeed(ip, PingTime::PING_FAILED, 0);
            *it = PingTime::PING_FAILED;
        }
    }
    else
    {
        Q_ASSERT(false);
    }

    if (isAllPingsFinished())
    {
        qCDebug(LOG_BEST_LOCATION()) << "Pings finished";
        int bestLocationInd;
        int bestLocationSelNodeInd;
        int bestLocationTimeMs;
        if (detectBestLocation(serverLocations_, bestLocationInd, bestLocationSelNodeInd, bestLocationTimeMs))
        {
            if (isNeedChangeBestLocation(bestLocationSelNodeInd, bestLocationTimeMs))
            {
                bestLocation_.set(serverLocations_[bestLocationInd]->getId(), serverLocations_[bestLocationInd]->getNodes()[bestLocationSelNodeInd].getHostname(), bestLocationTimeMs);
                outputBestLocationToLog(bestLocationInd, bestLocationSelNodeInd, bestLocationTimeMs);
            }

            QVector< QSharedPointer<ServerLocation> > serverLocationsWithBestLocation;

            QSharedPointer<ServerLocation> bestServerLocation = createBestLocation();
            if (!bestServerLocation.isNull())
            {
                serverLocationsWithBestLocation << bestServerLocation;
            }

            for (auto it = serverLocations_.begin(); it != serverLocations_.end(); it++)
            {
                 QSharedPointer<ServerLocation> slCopy(new ServerLocation);
                 *slCopy = **it;
                 serverLocationsWithBestLocation << slCopy;
            }
            emit serverLocationsAnswer(SERVER_RETURN_SUCCESS, serverLocationsWithBestLocation, forceDisconnectNodes_, userRole_);
        }
        else
        {
            qCDebug(LOG_BEST_LOCATION()) << "Can't detect best location after all pings finished";
            qCDebug(LOG_BEST_LOCATION()) << "Choose the random best location";
            // select random best location as a last chance
            selectRandomBestLocation();
        }
        pings_.clear();
    }
}

bool ServerLocationsApiWrapper::detectBestLocation(QVector<QSharedPointer<ServerLocation> > &serverLocations, int &outInd, int &outSelNodeInd, int &outTimeMs)
{
    int minAverageLatency = INT_MAX;
    int minAverageLatencyInd;
    int minAverageLatencyIndNode;
    int minNodeSpeed;
    bool isDetectedAtLeastOneLocation = false;

    int ind = 0;
    Q_FOREACH(QSharedPointer<ServerLocation> sl, serverLocations)
    {
        int averageLatency;
        int minLatency;
        int indNodeWithMinLatency;
        if (averageLatencyForLocation(sl.data(), averageLatency, minLatency, indNodeWithMinLatency))
        {
            if (averageLatency < minAverageLatency)
            {
                minAverageLatency = averageLatency;
                minAverageLatencyInd = ind;
                minAverageLatencyIndNode = indNodeWithMinLatency;
                minNodeSpeed = minLatency;
                isDetectedAtLeastOneLocation = true;
            }
        }
        ind++;
    }

    if (isDetectedAtLeastOneLocation)
    {
        outInd = minAverageLatencyInd;
        outSelNodeInd = minAverageLatencyIndNode;
        outTimeMs = minNodeSpeed;
        return true;
    }
    else
    {
        return false;
    }
}

bool ServerLocationsApiWrapper::averageLatencyForLocation(ServerLocation *sl, int &outAverageLatency, int &outMinLatency, int &outIndNodeWithMinLatency)
{
    QVector<ServerNode> &nodes = sl->getNodes();
    int sum = 0;
    int cntNodesHavePingData = 0;
    outMinLatency = INT_MAX;
    int nodeInd = 0;
    Q_FOREACH(const ServerNode &sn, nodes)
    {
        PingTime nodeSpeed = nodesSpeedStore_->getNodeSpeed(sn.getIpForPing());
        if (nodeSpeed != PingTime::NO_PING_INFO && nodeSpeed != PingTime::PING_FAILED)
        {
            sum += nodeSpeed.toInt();
            cntNodesHavePingData++;

            if (nodeSpeed.toInt() < outMinLatency)
            {
                outMinLatency = nodeSpeed.toInt();
                outIndNodeWithMinLatency = nodeInd;
            }
        }
        nodeInd++;
    }

    if (cntNodesHavePingData > 0)
    {
        outAverageLatency = sum / cntNodesHavePingData;
        return true;
    }
    else
    {
        return false;
    }
}

bool ServerLocationsApiWrapper::isAllPingsFinished()
{
    for (auto it = pings_.begin(); it != pings_.end(); ++it)
    {
        if (*it == PingTime::NO_PING_INFO)
        {
            return false;
        }
    }
    return true;
}

bool ServerLocationsApiWrapper::isNeedChangeBestLocation(int selectedNodeInd, PingTime pingTimeMs)
{
    Q_UNUSED(selectedNodeInd);
    // if prev best location not exist, then need to change
    if (!bestLocation_.isValid())
    {
        return true;
    }

    // check if bestLocation_ is removed from serverLocations_
    bool bFound = false;
    int findedInd = 0;
    for (auto it = serverLocations_.begin(); it != serverLocations_.end(); ++it)
    {
        if ((*it)->getId() == bestLocation_.getId())
        {
            bFound = true;
            break;
        }
        findedInd++;
    }
    if (!bFound)
    {
        return true;
    }

    // check if hostname not exist in location nodes
    int nodeInd = serverLocations_[findedInd]->nodeIndByHostname(bestLocation_.getSelectedHostname());
    if (nodeInd == -1)
    {
        return true;
    }

    PingTime timeMsForOldBestLocation = nodesSpeedStore_->getNodeSpeed(serverLocations_[findedInd]->getNodes()[nodeInd].getIpForPing());

    qCDebug(LOG_BEST_LOCATION()) << "Old best location ping time:" << timeMsForOldBestLocation.toInt() << "; new best location ping time:" << pingTimeMs.toInt();
    // check if best location changed and ping time changed more than 10%
    if (pingTimeMs.toInt() >= 0 && (double)pingTimeMs.toInt() < ((double)timeMsForOldBestLocation.toInt() * 0.9))
    {
        return true;
    }
    return false;
}

QSharedPointer<ServerLocation> ServerLocationsApiWrapper::createBestLocation()
{
    Q_ASSERT(bestLocation_.isValid());
    // find location index by id
    int ind = 0;
    for (auto it = serverLocations_.begin(); it != serverLocations_.end(); ++it)
    {
        if ((*it)->getId() == bestLocation_.getId())
        {
            break;
        }
        ind++;
    }

    int selNodeInd = serverLocations_[ind]->nodeIndByHostname(bestLocation_.getSelectedHostname());
    if (selNodeInd == -1)
    {
        Q_ASSERT(false);
        return QSharedPointer<ServerLocation>();
    }

    QSharedPointer<ServerLocation> bestServerLocation(new ServerLocation);
    *bestServerLocation = *serverLocations_[ind];
    bestServerLocation->transformToBestLocation(selNodeInd, bestLocation_.getPingMs(), bestLocation_.getId());
    return bestServerLocation;
}

void ServerLocationsApiWrapper::selectRandomBestLocation()
{
    // select the first location with nodes count > 0
    for (int i = 0; i < serverLocations_.count(); i++)
    {
        if (serverLocations_[i]->getNodes().count() > 0)
        {
            bestLocation_.set(serverLocations_[0]->getId(), serverLocations_[0]->getNodes()[0].getHostname(), PingTime::NO_PING_INFO);
            QVector< QSharedPointer<ServerLocation> > serverLocationsWithBestLocation;

            QSharedPointer<ServerLocation> bestServerLocation = createBestLocation();
            if (!bestServerLocation.isNull())
            {
                serverLocationsWithBestLocation << bestServerLocation;
            }

            for (auto it = serverLocations_.begin(); it != serverLocations_.end(); it++)
            {
                 QSharedPointer<ServerLocation> slCopy(new ServerLocation);
                 *slCopy = **it;
                 serverLocationsWithBestLocation << slCopy;
            }
            emit serverLocationsAnswer(SERVER_RETURN_SUCCESS, serverLocationsWithBestLocation, forceDisconnectNodes_, userRole_);
            return;
        }
    }
}

void ServerLocationsApiWrapper::outputBestLocationToLog(int ind, int selectedNodeInd, PingTime pingTimeMs)
{
    if (bestLocation_.isValid())
    {
        qCDebug(LOG_BEST_LOCATION()) << "Best location detected:" << serverLocations_[ind]->getName() <<
                                        "; Selected node IP:" << serverLocations_[ind]->getNodes()[selectedNodeInd].getIpForPing() <<
                                        "; Ping time ms:" << pingTimeMs.toInt();
    }
}
