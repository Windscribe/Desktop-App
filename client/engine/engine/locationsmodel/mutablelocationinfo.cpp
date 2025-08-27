#include "mutablelocationinfo.h"

#include "utils/ws_assert.h"
#include "utils/log/categories.h"
#include "utils/ipvalidation.h"
#include "utils/utils.h"
#include "nodeselectionalgorithm.h"

namespace locationsmodel {

MutableLocationInfo::MutableLocationInfo(const LocationID &locationId, const QString &name, const QVector< QSharedPointer<const BaseNode> > &nodes, int selectedNode,
                                         const QString &dnsHostName, const QString &verifyX509name)
    : BaseLocationInfo(locationId, name)
    , nodes_(nodes)
    , selectedNode_(selectedNode)
    , dnsHostName_(dnsHostName)
    , verifyX509name_(verifyX509name)
{

    QString strNodes;
    for (int i = 0; i < nodes_.count(); ++i)
    {
        strNodes += nodes_[i]->getIp(0);
        strNodes += "; ";
    }

    qCDebug(LOG_BASIC) << "MutableLocationInfo created: " << name << strNodes << "; Selected node:" << selectedNode_;
}


QString MutableLocationInfo::getDnsName() const
{
    if (locationId_.isStaticIpsLocation())
    {
        if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
        {
            return nodes_[selectedNode_]->getStaticIpDnsName();
        }
        else
        {
            WS_ASSERT(false);
            return "";
        }
    }
    else
    {
        return dnsHostName_;
    }
}

bool MutableLocationInfo::isExistSelectedNode() const
{
    return selectedNode_ != -1;
}

QString MutableLocationInfo::getVerifyX509name() const
{
    return verifyX509name_;
}

QString MutableLocationInfo::getLogString() const
{
    QString strNodes;
    for (int i = 0; i < nodes_.count(); ++i)
    {
        strNodes += getLogForNode(i);
        strNodes += "; ";
    }

    return "Location nodes: " + strNodes;
}

int MutableLocationInfo::nodesCount() const
{
    return nodes_.count();
}

QString MutableLocationInfo::getIpForNode(int indNode, int indIp) const
{
    WS_ASSERT(indNode >= 0 && indNode < nodes_.count());
    WS_ASSERT(indIp >= 0 && indIp <= 3);
    return nodes_[indNode]->getIp(indIp);
}

// goto next node or to first (if current selected last or incorrect)
void MutableLocationInfo::selectNextNode()
{
    selectedNode_ ++;
    if (selectedNode_ >= nodes_.count())
    {
        WS_ASSERT(nodes_.count() > 0);
        selectedNode_ = 0;
    }
}

void MutableLocationInfo::selectNodeByIp(const QString &addr)
{
    WS_ASSERT(IpValidation::isIp(addr));
    for (int i = 0; i < nodes_.count(); i++) {
        for (int j = 0; j < 3; j++) {
            if (nodes_[i]->getIp(j) == addr) {
                qCDebug(LOG_BASIC) << "Selected node by IP: " << i;
                selectedNode_ = i;
                return;
            }
        }
    }
    qCWarning(LOG_BASIC) << "Could not find node for IP: " << addr;
}

QString MutableLocationInfo::getIpForSelectedNode(int indIp) const
{
    WS_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getIp(indIp);
}

QString MutableLocationInfo::getHostnameForSelectedNode() const
{
    WS_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getHostname();
}

QString MutableLocationInfo::getWgPubKeyForSelectedNode() const
{
    WS_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getWgPubKey();
}

QString MutableLocationInfo::getWgIpForSelectedNode() const
{
    WS_ASSERT(locationId_.isStaticIpsLocation());
    WS_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_]->getWgIp();
}

QString MutableLocationInfo::getStaticIpUsername() const
{
    WS_ASSERT(locationId_.isStaticIpsLocation());
    if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_]->getStaticIpUsername();
    }
    else
    {
        WS_ASSERT(false);
        return "";
    }
}

QString MutableLocationInfo::getStaticIpPassword() const
{
    WS_ASSERT(locationId_.isStaticIpsLocation());
    if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_]->getStaticIpPassword();
    }
    else
    {
        WS_ASSERT(false);
        return "";
    }
}

api_responses::StaticIpPortsVector MutableLocationInfo::getStaticIpPorts() const
{
    WS_ASSERT(locationId_.isStaticIpsLocation());
    if (selectedNode_ >= 0 && selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_]->getStaticIpPorts();
    }
    else
    {
        WS_ASSERT(false);
        return api_responses::StaticIpPortsVector();
    }
}

QString MutableLocationInfo::getLogForNode(int ind) const
{
    QString ret = "node" + QString::number(ind + 1);

    ret += " = {";

    const int IPS_COUNT = 3;
    for (int i = 0; i < IPS_COUNT; ++i)
    {
        ret += "ip" + QString::number(i + 1) + " = ";
        ret += getIpForNode(ind, i);
        if (i != (IPS_COUNT - 1))
        {
            ret += ", ";
        }
    }

    ret += "}";
    return ret;
}


/*void MutableLocationInfo::locationChanged(const LocationID &locationId, const QVector<ServerNode> &nodes, const QString &dnsHostName)
{
    WS_ASSERT(threadHandle_ == QThread::currentThreadId());

    if (locationId == locationId_ && nodes.count() > 0)
    {
        WS_ASSERT(selectedNode_ != -1);
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

