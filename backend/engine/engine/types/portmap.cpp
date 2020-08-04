#include "portmap.h"

#include <QDataStream>

const PortItem *PortMap::getPortItemByHeading(const QString &heading)
{
    Q_FOREACH(const PortItem &portItem, items)
    {
        if (portItem.heading == heading)
        {
            return &portItem;
        }
    }
    return NULL;
}

const PortItem *PortMap::getPortItemByProtocolType(const ProtocolType &protocol)
{
    Q_FOREACH(const PortItem &portItem, items)
    {
        if (portItem.protocol.isEqual(protocol))
        {
            return &portItem;
        }
    }
    return NULL;
}

int PortMap::getUseIpInd(const ProtocolType &connectionProtocol)
{
    Q_FOREACH(const PortItem &portItem, items)
    {
        if (portItem.protocol.isEqual(connectionProtocol))
        {
            if (portItem.use == "ip")
                return 0;
            else if (portItem.use == "ip2")
                return 1;
            else if (portItem.use == "ip3")
                return 2;
            // for ikev2 protocol, use 0 ind for ip
            else if (portItem.use == "hostname")
                return 0;
            else
            {
                Q_ASSERT(false);
                return -1;
            }
        }
    }
    Q_ASSERT(false);
    return -1;
}

uint PortMap::getLegacyPort(const ProtocolType &connectionProtocol)
{
    Q_FOREACH(const PortItem &portItem, items)
    {
        if (portItem.protocol.isEqual(connectionProtocol))
        {
            return portItem.legacy_port;
        }
    }
    Q_ASSERT(false);
    return ~0u;
}

void PortMap::writeToStream(QDataStream &stream)
{
    stream << items.count();
    Q_FOREACH(const PortItem &pi, items)
    {
        pi.writeToStream(stream);
    }
}

void PortMap::readFromStream(QDataStream &stream)
{
    int cnt;
    stream >> cnt;
    for (int i = 0; i < cnt; ++i)
    {
        PortItem pi;
        pi.readFromStream(stream);
        items << pi;
    }
}

void PortItem::writeToStream(QDataStream &stream) const
{
    stream << protocol.toLongString();
    stream << heading;
    stream << use;
    stream << ports;
    stream << legacy_port;
}

void PortItem::readFromStream(QDataStream &stream)
{
    QString protocolStr;
    stream >> protocolStr;
    protocol = ProtocolType(protocolStr);
    stream >> heading;
    stream >> use;
    stream >> ports;
    stream >> legacy_port;
}
