#include "mutablelocationinfo.h"

#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "utils/utils.h"
#include "engine/dnsresolver/dnsresolver.h"
#include "nodeselectionalgorithm.h"

namespace locationsmodel {

MutableLocationInfo::MutableLocationInfo(const LocationID &locationId, const QString &name, const QVector< QSharedPointer<BaseNode> > &nodes, int selectedNode,
                                         const QString &dnsHostName)
    : QObject(nullptr), locationId_(locationId),
      name_(name), nodes_(nodes), dnsHostName_(dnsHostName)
{
    if (selectedNode == -1)
    {
        // select random node based on weight
        if (nodes.count() > 0)
        {
            selectedNode = NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(nodes);
            Q_ASSERT(selectedNode >= 0 && selectedNode < nodes.count());
        }
    }
    selectedNode_ = selectedNode;

    QString strNodes;
    for (int i = 0; i < nodes_.count(); ++i)
    {
        strNodes += nodes_[i]->getIp(0);
        strNodes += "; ";
    }

    qCDebug(LOG_BASIC) << "MutableLocationInfo created: " << name << strNodes << "; Selected node:" << selectedNode_;
}


bool MutableLocationInfo::isCustomOvpnConfig() const
{
    return locationId_.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID;
}

bool MutableLocationInfo::isStaticIp() const
{
    return locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID;
}

QString MutableLocationInfo::getName() const
{
    return name_;
}

QString MutableLocationInfo::getDnsName() const
{
    if (isStaticIp())
    {
        if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
        {
            return nodes_[selectedNode_]->getStaticIpDnsName();
        }
        else
        {
            Q_ASSERT(false);
            return "";
        }
    }
    else
    {
        return dnsHostName_;
    }
}

QString MutableLocationInfo::getCustomOvpnConfigPath() const
{
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getCustomOvpnConfigPath();
}

bool MutableLocationInfo::isExistSelectedNode() const
{
    return selectedNode_ != -1;
}

int MutableLocationInfo::nodesCount() const
{
    return nodes_.count();
}

QString MutableLocationInfo::getIpForNode(int indNode, int indIp) const
{
    Q_ASSERT(indNode >= 0 && indNode < nodes_.count());
    Q_ASSERT(indIp >= 0 && indIp <= 3);
    return nodes_[indNode]->getIp(indIp);
}

// goto next node or to first (if current selected last or incorrect)
void MutableLocationInfo::selectNextNode()
{
    selectedNode_ ++;
    if (selectedNode_ >= nodes_.count())
    {
        Q_ASSERT(nodes_.count() > 0);
        selectedNode_ = 0;
    }
}

QString MutableLocationInfo::getIpForSelectedNode(int indIp) const
{
    Q_ASSERT(indIp >= 0 && indIp <= 3);
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getIp(indIp);
}

QString MutableLocationInfo::getHostnameForSelectedNode() const
{
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getHostname();
}

QString MutableLocationInfo::getWgPubKeyForSelectedNode() const
{
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getWgPubKey();
}

QString MutableLocationInfo::getWgIpForSelectedNode() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getWgIp();
}

QString MutableLocationInfo::getStaticIpUsername() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_]->getStaticIpUsername();
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

QString MutableLocationInfo::getStaticIpPassword() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_]->getStaticIpPassword();
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

apiinfo::StaticIpPortsVector MutableLocationInfo::getStaticIpPorts() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_]->getStaticIpPorts();
    }
    else
    {
        Q_ASSERT(false);
        return apiinfo::StaticIpPortsVector();
    }
}

/*QString MutableLocationInfo::resolveHostName()
{
    /*Q_ASSERT(locationId_.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID);
    Q_ASSERT(nodes_.count() == 1);

    if (locationId_.getId() != LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID || nodes_.count() != 1)
    {
        qCDebug(LOG_BASIC) << "MutableLocationInfo::resolveHostName() error, not correct location id or node count != 1";
        return "";
    }

    if (IpValidation::instance().isIp(nodes_[0].getHostname()))
    {
        nodes_[0].setIpForCustomOvpnConfig(nodes_[0].getHostname());
        return nodes_[0].getHostname();
    }
    else
    {
        QHostInfo hostInfo = DnsResolver::instance().lookupBlocked(nodes_[0].getHostname(), true);
        if (hostInfo.error() == QHostInfo::NoError && hostInfo.addresses().count() > 0)
        {
            qCDebug(LOG_BASIC) << "MutableLocationInfo, resolved IP address for" << nodes_[0].getHostname() << ":" << hostInfo.addresses()[0];
            QString ip = hostInfo.addresses()[0].toString();
            nodes_[0].setIpForCustomOvpnConfig(ip);
            return ip;
        }
        else
        {
            qCDebug(LOG_BASIC) << "MutableLocationInfo, failed to resolve" << nodes_[0].getHostname();
            return "";
        }
    }*/
/*    return "";
}

void MutableLocationInfo::locationChanged(const LocationID &locationId, const QVector<ServerNode> &nodes, const QString &dnsHostName)
{
    /*Q_ASSERT(threadHandle_ == QThread::currentThreadId());

    if (locationId == locationId_ && nodes.count() > 0)
    {
        Q_ASSERT(selectedNode_ != -1);
        ServerNode prevSelectedNode = nodes_[selectedNode_];

        nodes_ = nodes;
        dnsHostName_ = dnsHostName;

        selectedNode_ = 0; // by default select first node
        for (int i = 0; i < nodes_.count(); ++i)
        {
            if (nodes_[i].isEqualIpsAndHostnameAndLegacy(prevSelectedNode))
            {
                selectedNode_ = i;
                break;
            }
        }


        QString strNodes;
        for (int i = 0; i < nodes_.count(); ++i)
        {
            strNodes += nodes_[i].getIpForPing();
            strNodes += "; ";
        }
        qCDebug(LOG_BASIC) << "MutableLocationInfo locationChanged: " << locationId.getHashString() << strNodes << "; Selected node:" << selectedNode_;
    }
}*/

} //namespace locationsmodel
