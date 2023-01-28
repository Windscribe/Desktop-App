#include "pingmanager.h"
#include <QTimer>
#include <QThread>
#include "nodeselectionalgorithm.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"

PingManager::PingManager(QObject *parent, NodesSpeedRatings *nodesSpeedRating, NodesSpeedStore *nodesSpeedStore, IConnectStateController *stateController, INetworkStateManager *networkStateManager) : QObject(parent),
    isInitialized_(false),
    nodesSpeedRating_(nodesSpeedRating),
    nodesSpeedStore_(nodesSpeedStore),
    mutex_(QRecursiveMutex()),
    isBestLocationDetermined_(false),
    bestLocationId_(0),
    selectedNodeIndForBestLocation_(0),
    pingNodesController_(NULL),
    stateController_(stateController),
    networkStateManager_(networkStateManager),
    pingLog_("ping_log.txt")
{
    connect(nodesSpeedRating, SIGNAL(nodeSpeedRatingChanged(QString)), SLOT(onNodeSpeedRatingChanged(QString)));
}

PingManager::~PingManager()
{
}

bool PingManager::isInitialized()
{
    QMutexLocker locker(&mutex_);
    return isInitialized_;
}

void PingManager::updateServers(QVector<QSharedPointer<ServerLocation> > &newServers, bool bSkipCustomOvpnLocation)
{
    QMutexLocker locker(&mutex_);

    pingLog_.addLog("PingManager::updateNodes", "updateServers");

    for (auto it = hashLocationsSpeedInfo_.begin(); it != hashLocationsSpeedInfo_.end(); ++it)
    {
        it.value()->existThisLocation = false;
    }

    isBestLocationDetermined_ = false;

    QVector<PingNodesController::PingNodeAndType> allIps;

    for (QSharedPointer<ServerLocation> sl : qAsConst(newServers))
    {
        // skip custom ovpn configs location
        if (bSkipCustomOvpnLocation && sl->getType() == ServerLocation::SERVER_LOCATION_CUSTOM_CONFIG)
        {
            continue;
        }
        // best location store separately
        if (sl->getType() == ServerLocation::SERVER_LOCATION_BEST)
        {
            bestLocationId_ = sl->getBestLocationId();
            selectedNodeIndForBestLocation_ = sl->getBestLocationSelectedNodeInd();
            isBestLocationDetermined_ = true;

            qCDebug(LOG_BASIC) << "PingManager::updateServers, selectedNode for best location:" << selectedNodeIndForBestLocation_ << "; id:" << sl->getBestLocationId();
        }
        else // not best location
        {
            // first handle cities (order is important, because root locations use city locations for calc average latency speed)
            const QStringList cities = sl->getCities();
            for (const QString &cityName: cities)
            {
                LocationID cityLocationId(sl->getId(), cityName);
                hashNames_[cityLocationId] = cityName;
                QVector<ServerNode> cityNodes = sl->getCityNodes(cityName);
                addLocationOrUpdate(cityLocationId, cityNodes);
                hashLocationsSpeedInfo_[cityLocationId]->existThisLocation = true;
            }

            // handle root locations
            LocationID locationId(sl->getId());
            hashNames_[locationId] = sl->getName();
            QVector<ServerNode> &nodes = sl->getNodes();
            addLocationOrUpdate(locationId, nodes);

            hashLocationsSpeedInfo_[locationId]->existThisLocation = true;

            // store all ips in list
            for (const ServerNode &sn: qAsConst(nodes))
            {
                PingNodesController::PingNodeAndType pn;
                pn.ip = sn.getIpForPing();
                pn.pingType = (sl->getType() == ServerLocation::SERVER_LOCATION_CUSTOM_CONFIG) ? PingHost::PING_ICMP : PingHost::PING_TCP;
                allIps << pn;
            }
        }
    }

    // remove unused location
    QHash<LocationID, QSharedPointer<LocationSpeedInfo> >::iterator it = hashLocationsSpeedInfo_.begin();
    while (it != hashLocationsSpeedInfo_.end())
    {
        if (!it.value()->existThisLocation)
        {
            pingLog_.addLog("PingManager::updateNodes", "removed unused location: " + it.key().getHashString());
            it = hashLocationsSpeedInfo_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    QMetaObject::invokeMethod(pingNodesController_, "updateNodes", Qt::QueuedConnection, Q_ARG(QVector<PingNodesController::PingNodeAndType>, allIps));
}

void PingManager::clear()
{
    QMutexLocker locker(&mutex_);
    hashLocationsSpeedInfo_.clear();
    hashNames_.clear();
    QVector<PingNodesController::PingNodeAndType> emptyVector;
    QMetaObject::invokeMethod(pingNodesController_, "updateNodes", Qt::QueuedConnection, Q_ARG(QVector<PingNodesController::PingNodeAndType>, emptyVector));

}

void PingManager::setProxySettings(const ProxySettings &proxySettings)
{
    QMutexLocker locker(&mutex_);
    if (pingNodesController_)
    {
        pingNodesController_->setProxySettings(proxySettings);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void PingManager::disableProxy()
{
    QMutexLocker locker(&mutex_);
    if (pingNodesController_)
    {
        pingNodesController_->disableProxy();
    }
    else
    {
        Q_ASSERT(false);
    }
}

void PingManager::enableProxy()
{
    QMutexLocker locker(&mutex_);
    if (pingNodesController_)
    {
        pingNodesController_->enableProxy();
    }
    else
    {
        Q_ASSERT(false);
    }
}

void PingManager::onPingStartedInDisconnectedState(const QString &ip)
{
    for (auto it = hashLocationsSpeedInfo_.begin(); it != hashLocationsSpeedInfo_.end(); ++it)
    {
        int ind = 0;
        for (const ServerNode &sn : qAsConst(it.value()->nodes))
        {
            if (sn.getIpForPing() == ip)
            {
                it.value()->pingInfo[ind].iteration = 0;
            }
            ++ind;
        }
    }
}

PingTime PingManager::getPingTimeMs(const LocationID &locationId)
{
    QMutexLocker locker(&mutex_);

    LocationID lid = locationId;
    // this is best location?
    if (locationId.getId() == LocationID::BEST_LOCATION_ID)
    {
        // TODO: ?
        // Q_ASSERT(isBestLocationDetermined_);
        lid.setId(bestLocationId_);
    }

    auto it = hashLocationsSpeedInfo_.find(lid);
    if (it != hashLocationsSpeedInfo_.end())
    {
        return it.value()->timeMs;
    }
    else
    {
        //Q_ASSERT(false);
        return PingTime::PING_FAILED;
    }
}

int PingManager::getSelectedNode(const LocationID &locationId)
{
    QMutexLocker locker(&mutex_);

    if (locationId.getId() == LocationID::BEST_LOCATION_ID && locationId.getCity().isEmpty() && isBestLocationDetermined_)
    {
        return selectedNodeIndForBestLocation_;
    }

    LocationID lid = locationId;
    // this is best location with city?
    if (locationId.getId() == LocationID::BEST_LOCATION_ID)
    {
        Q_ASSERT(isBestLocationDetermined_);
        lid.setId(bestLocationId_);
    }

    auto it = hashLocationsSpeedInfo_.find(lid);
    if (it != hashLocationsSpeedInfo_.end())
    {
        return it.value()->selectedNode;
    }
    else
    {
        Q_ASSERT(false);
        qCDebug(LOG_BASIC) << "PingManager::getSelectedNode Q_ASSERT(false)";
        return -1;
    }
}

void PingManager::init()
{
    QMutexLocker locker(&mutex_);

    pingNodesController_ = new PingNodesController(this, stateController_, nodesSpeedStore_, networkStateManager_, pingLog_);
    connect(pingNodesController_, SIGNAL(pingInfoChanged(QString,int, quint32)), SLOT(onPingInfoChanged(QString,int, quint32)));
    connect(pingNodesController_, SIGNAL(pingStartedInDisconnectedState(QString)), SLOT(onPingStartedInDisconnectedState(QString)));

    isInitialized_ = true;
}

void PingManager::onPingInfoChanged(const QString &ip, int timems, quint32 iteration)
{
    // first for cities
    handlePingInfoChanged(ip, timems, iteration, true);
    // second for root locations, because root locations use city location for average latency
    handlePingInfoChanged(ip, timems, iteration, false);
}

void PingManager::onNodeSpeedRatingChanged(const QString &hostname)
{
    pingLog_.addLog("PingManager::onNodeSpeedRatingChanged", "User changed speed rating for hostname: " + hostname);
    for (auto it = hashLocationsSpeedInfo_.begin(); it != hashLocationsSpeedInfo_.end(); ++it)
    {
        int ind = 0;
        for (const ServerNode &sn : qAsConst(it.value()->nodes))
        {
            if (sn.getHostname() == hostname)
            {
                recalculateSpeed(it.key(), it.value().data());

                //todo: emit signal connectionSpeedChanged if need


                break;
            }
            ++ind;
        }
    }
}

void PingManager::addLocationOrUpdate(const LocationID &locationId, QVector<ServerNode> &nodes)
{
    auto it = hashLocationsSpeedInfo_.find(locationId);
    if (it != hashLocationsSpeedInfo_.end())
    {
       if (!isNodesVectorsEqual(it.value()->nodes, nodes))
        {
            pingLog_.addLog("PingManager::addLocationOrUpdate", hashNames_[locationId] + " nodes changed");

            bool bExistPrevSelectedNode = false;
            ServerNode prevSelectedNode;
            PingTime prevTimeMs;
            if (it.value()->selectedNode != -1)
            {
                prevSelectedNode = it.value()->nodes[it.value()->selectedNode];
                prevTimeMs = it.value()->timeMs;
                bExistPrevSelectedNode = true;
            }

            hashLocationsSpeedInfo_[locationId] = copyLocationSpeedInfoFromExisting(it.value().data(), nodes);

            LocationSpeedInfo *lsi = hashLocationsSpeedInfo_[locationId].data();
            if (!recalculateSpeed(locationId, lsi))
            {
                // if prev selected node exist, then select the same
                bool bUsedPrevSelectedNode = false;
                if (bExistPrevSelectedNode)
                {
                    int ind = lsi->findNodeInd(prevSelectedNode);
                    if (ind != -1)
                    {
                        lsi->selectedNode = ind;
                        lsi->timeMs = prevTimeMs;
                        bUsedPrevSelectedNode = true;

                        pingLog_.addLog("PingManager::addLocationOrUpdate", hashNames_[locationId] + "; sel_node the same");
                    }
                }


                // default values, if there is not enough ping data and/or prev selected node removed
                if (!bUsedPrevSelectedNode)
                {
                    lsi->timeMs = PingTime::NO_PING_INFO;
                    QVector<double> p;
                    // (random node selection based on server weight and user rating)
                    lsi->selectedNode = NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(lsi->nodes, nodesSpeedRating_, p);

                    QString logStr;
                    for (int i = 0; i < lsi->nodes.count(); ++i)
                    {
                        logStr += lsi->nodes[i].getIpForPing() + " ";
                        logStr += QString::number(p[i], 'g', 2);
                        logStr += "; ";
                    }

                    pingLog_.addLog("PingManager::addLocationOrUpdate", hashNames_[locationId] + "; sel_node:" + QString::number(lsi->selectedNode) + " ;" + logStr);
                }
            }
            emitConnectionSpeedChangedWrapper(locationId, lsi->timeMs);
        }
        else
        {
            recalculateSpeed(locationId, it.value().data());
            emitConnectionSpeedChangedWrapper(locationId, it.value()->timeMs);
        }
    }
    else
    {
        // add new location to mapLocationsSpeedInfo_
        QSharedPointer<LocationSpeedInfo> lsi(new LocationSpeedInfo);
        lsi->nodes = nodes;

        lsi->pingInfo.reserve(lsi->nodes.count());
        for (int i = 0; i < lsi->nodes.count(); ++i)
        {
            PingInfo pi;
            pi.isExists = false;
            lsi->pingInfo << pi;
        }

        selectSpeedAndSelectedNodeForNewLocation(locationId, lsi.data());
        hashLocationsSpeedInfo_[locationId] = lsi;
        emitConnectionSpeedChangedWrapper(locationId, lsi->timeMs);
    }
}

void PingManager::selectSpeedAndSelectedNodeForNewLocation(const LocationID &locationId, PingManager::LocationSpeedInfo *lsi)
{
    bool bAllNodesHavePingTime = true;
    int ind = 0;

    if (lsi->nodes.count() == 0)
    {
        lsi->selectedNode = -1;
        lsi->timeMs = PingTime::PING_FAILED;
        return;
    }

    for (const ServerNode &sn : qAsConst(lsi->nodes))
    {
        PingTime timeMs = nodesSpeedStore_->getNodeSpeed(sn.getIpForPing());

        if (timeMs == PingTime::NO_PING_INFO)
        {
            bAllNodesHavePingTime = false;
            break;
        }
        else
        {
            lsi->pingInfo[ind].isExists = true;
            lsi->pingInfo[ind].timeMs = timeMs;
            lsi->pingInfo[ind].iteration = 0;
        }
        ++ind;
    }

    if (bAllNodesHavePingTime)
    {
        recalculateSpeed(locationId, lsi);
    }
    else
    {
        // default values, if there is not enough ping data
        lsi->timeMs = PingTime::NO_PING_INFO;
        QVector<double> p;
        // (random node selection based on server weight and user rating)
        lsi->selectedNode = NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(lsi->nodes, nodesSpeedRating_, p);

        QString logStr;
        for (int i = 0; i < lsi->nodes.count(); ++i)
        {
            logStr += lsi->nodes[i].getIpForPing() + " ";
            logStr += QString::number(p[i], 'g', 2);
            logStr += "; ";
        }

        pingLog_.addLog("PingManager::newNodeWithoutPingInfo", hashNames_[locationId] + "; sel_node:" + QString::number(lsi->selectedNode) + " ;" + logStr);
    }
}

bool PingManager::recalculateSpeed(const LocationID &locationId, PingManager::LocationSpeedInfo *lsi)
{
    if (lsi->isAllNodesHavePingWithSameIteration())
    {
        QVector<NodeSelectionAlgorithm::NodeInfo> nodeInfos;
        for (int i = 0; i < lsi->nodes.count(); ++i)
        {
            NodeSelectionAlgorithm::NodeInfo ni;
            int rating;
            if (nodesSpeedRating_->getRatingForNode(lsi->nodes[i].getHostname(), rating))
            {
                ni.userRating = rating;
            }
            else
            {
                ni.userRating = 0;
            }
            ni.weight = lsi->nodes[i].getWeight();
            ni.pingTime = lsi->pingInfo[i].timeMs.toInt();
            nodeInfos << ni;
        }

        QVector<double> p;
        int indSel = NodeSelectionAlgorithm::selectRandomNodeBasedOnLatency(nodeInfos, p);
        if (indSel == -1)
        {
            lsi->selectedNode = -1;
            lsi->timeMs = PingTime::PING_FAILED;
        }
        else
        {
            //lsi->connectionSpeed = timeMsToConnectionSpeed(lsi->pingInfo[indSel].timeMs);
            lsi->selectedNode = indSel;

            // this is root location
            if (locationId.getCity().isEmpty())
            {
                // select average latency from city locations
                QVector<int> citiesLatencies;
                for (auto it = hashLocationsSpeedInfo_.begin(); it != hashLocationsSpeedInfo_.end(); ++it)
                {
                    if (it.key().getId() == locationId.getId() && (!it.key().getCity().isEmpty()))
                    {
                        citiesLatencies << it.value()->timeMs.toInt();
                    }
                }

                lsi->timeMs = getAverageLatencyFromCitiesLatencies(citiesLatencies);
            }
            // this is city location
            else
            {
                lsi->timeMs = lsi->pingInfo[indSel].timeMs;
            }
        }

        QString logStr;
        for (int i = 0; i < lsi->nodes.count(); ++i)
        {
            logStr += lsi->nodes[i].getIpForPing() + " ";
            logStr += QString::number(p[i], 'g', 2);
            logStr += "; ";
        }

        pingLog_.addLog("PingManager::recalculateSpeed", hashNames_[locationId] + "; sel_node:" + QString::number(lsi->selectedNode) + " ;" + logStr);

        return true;
    }
    else
    {
        return false;
    }
}

bool PingManager::isNodesVectorsEqual(const QVector<ServerNode> &nodes1, const QVector<ServerNode> &nodes2)
{
    if (nodes1.count() != nodes2.count())
    {
        return false;
    }
    else
    {
        for (int i = 0; i < nodes1.count(); ++i)
        {
            if (!nodes1[i].isEqual(nodes2[i]))
            {
                return false;
            }
        }
    }
    return true;
}

QSharedPointer<PingManager::LocationSpeedInfo> PingManager::copyLocationSpeedInfoFromExisting(PingManager::LocationSpeedInfo *lsi, QVector<ServerNode> &newNodes)
{
    QSharedPointer<LocationSpeedInfo> newLsi(new LocationSpeedInfo);

    newLsi->nodes = newNodes;
    newLsi->pingInfo.reserve(newLsi->nodes.size());
    for (int i = 0; i < newLsi->nodes.count(); ++i)
    {
        PingInfo pi;
        if (!lsi->findPingInfoForNode(newLsi->nodes[i], pi))
        {
            pi.isExists = false;
            pi.iteration = 0;
            pi.timeMs = -1;
        }
        newLsi->pingInfo << pi;
    }
    return newLsi;
}

int PingManager::getAverageLatencyFromCitiesLatencies(const QVector<int> &latencies)
{
    int sum = 0;
    int cnt = 0;
    for (int latency : latencies)
    {
        if (latency >= 0)
        {
            sum += latency;
            cnt++;
        }
    }

    if (cnt > 0)
    {
        return (int)((double)sum / (double)cnt + 0.5);
    }
    else
    {
        return -1;
    }
}

void PingManager::handlePingInfoChanged(const QString &ip, int timems, quint32 iteration, bool bForCities)
{
    for (auto it = hashLocationsSpeedInfo_.begin(); it != hashLocationsSpeedInfo_.end(); ++it)
    {
        if ((bForCities && !it.key().getCity().isEmpty()) || (!bForCities && it.key().getCity().isEmpty()))
        {

            bool bChangedLocation = false;
            int ind = 0;
            for (const ServerNode &sn : qAsConst(it.value()->nodes))
            {
                if (sn.getIpForPing() == ip)
                {
                    it.value()->pingInfo[ind].isExists = true;
                    it.value()->pingInfo[ind].timeMs = timems;
                    it.value()->pingInfo[ind].iteration = iteration;
                    bChangedLocation = true;
                }
                ++ind;
            }
            if (bChangedLocation)
            {
                PingTime wasTimeMs = it.value()->timeMs;
                recalculateSpeed(it.key(), it.value().data());
                if (it.value()->timeMs != wasTimeMs)
                {
                    emitConnectionSpeedChangedWrapper(it.key(), it.value()->timeMs);
                }
            }
        }
    }
}

void PingManager::emitConnectionSpeedChangedWrapper(LocationID locationId, PingTime timeMs)
{
    // wrapper for emit signal to best location too
    if (isBestLocationDetermined_ && locationId.getId() == bestLocationId_)
    {
        emit connectionSpeedChanged(LocationID(LocationID::BEST_LOCATION_ID, locationId.getCity()), timeMs);
    }
    emit connectionSpeedChanged(locationId, timeMs);
}

bool PingManager::LocationSpeedInfo::isAllNodesHavePingWithSameIteration() const
{
    if (pingInfo.count() > 0)
    {
        if (pingInfo[0].isExists)
        {
            quint32 iteration = pingInfo[0].iteration;
            for (int i = 1; i < pingInfo.count(); ++i)
            {
                if (!pingInfo[i].isExists || pingInfo[i].iteration != iteration)
                {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool PingManager::LocationSpeedInfo::findPingInfoForNode(const ServerNode &sn, PingManager::PingInfo &outPi) const
{
    Q_ASSERT(nodes.count() == pingInfo.count());
    for (int i = 0; i < nodes.count(); ++i)
    {
        if (nodes[i].getIpForPing() == sn.getIpForPing())
        {
            outPi = pingInfo[i];
            return true;
        }
    }
    return false;
}

int PingManager::LocationSpeedInfo::findNodeInd(const ServerNode &sn) const
{
    for (int i = 0; i < nodes.count(); ++i)
    {
        if (nodes[i].getIpForPing() == sn.getIpForPing())
        {
            return i;
        }
    }
    return -1;
}
