#include "mutablelocationinfo.h"

#include <QThread>
#include "Utils/logger.h"
#include "Utils/ipvalidation.h"
#include "Utils/utils.h"
#include "Engine/DnsResolver/dnsresolver.h"

MutableLocationInfo::MutableLocationInfo(const LocationID &locationId, const QString &name,
                                         const QVector<ServerNode> &nodes, int selectedNode,
                                         const QString &dnsHostName)
: QObject(NULL)
{
    threadHandle_ = QThread::currentThreadId();
    locationId_ = locationId;
    name_ = name;
    nodes_ = nodes;
    if (selectedNode == -1)
    {
        // select random node
        if (nodes.count() > 0)
        {
            selectedNode = Utils::generateIntegerRandom(0, nodes.count() - 1);
        }
    }
    selectedNode_ = selectedNode;
    dnsHostName_ = dnsHostName;

    QString strNodes;
    for (int i = 0; i < nodes_.count(); ++i)
    {
        strNodes += nodes_[i].getIpForPing();
        strNodes += "; ";
    }

    qCDebug(LOG_BASIC) << "MutableLocationInfo created: " << locationId.getHashString() << strNodes << "; Selected node:" << selectedNode_;
}

MutableLocationInfo::~MutableLocationInfo()
{
    //qDebug() << "MutableLocationInfo deleted: " << name_;
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
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    return name_;
}

QString MutableLocationInfo::getDnsName() const
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    if (isStaticIp())
    {
        if (selectedNode_ < nodes_.count())
        {
            return nodes_[selectedNode_].getStaticIpDnsName();
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
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_].getCustomOvpnConfigPath();
}

int MutableLocationInfo::isExistSelectedNode() const
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    return selectedNode_ != -1;
}

const ServerNode &MutableLocationInfo::getSelectedNode() const
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    Q_ASSERT(selectedNode_ >= 0 && selectedNode_ < nodes_.count());
    return nodes_[selectedNode_];
}

int MutableLocationInfo::nodesCount() const
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    return nodes_.count();
}

const ServerNode &MutableLocationInfo::getNode(int ind) const
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    return nodes_[ind];
}

// goto next node or to first (if current selected last or incorrect)
void MutableLocationInfo::selectNextNode()
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());
    selectedNode_ ++;
    if (selectedNode_ >= nodes_.count())
    {
        Q_ASSERT(nodes_.count() > 0);
        selectedNode_ = 0;
    }
}

QString MutableLocationInfo::getStaticIpUsername() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    if (selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_].getUsername();
    }
    else
    {
        return "";
    }
}

QString MutableLocationInfo::getStaticIpPassword() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    if (selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_].getPassword();
    }
    else
    {
        return "";
    }
}

StaticIpPortsVector MutableLocationInfo::getStaticIpPorts() const
{
    Q_ASSERT(locationId_.getId() == LocationID::STATIC_IPS_LOCATION_ID);
    if (selectedNode_ < nodes_.count())
    {
        return nodes_[selectedNode_].getAllStaticIpIntPorts();
    }
    else
    {
        return StaticIpPortsVector();
    }
}

QString MutableLocationInfo::resolveHostName()
{
    Q_ASSERT(locationId_.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID);
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
    }
}

void MutableLocationInfo::locationChanged(const LocationID &locationId, const QVector<ServerNode> &nodes, const QString &dnsHostName)
{
    Q_ASSERT(threadHandle_ == QThread::currentThreadId());

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
}
