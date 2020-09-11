#include "portmap.h"

#include <QDataStream>
#include <QJsonObject>

const int typeIdPortMap = qRegisterMetaType<ApiInfo::PortMap>("ApiInfo::PortMap");

namespace ApiInfo {

bool PortMap::initFromJson(const QJsonArray &jsonArray)
{
    for (const QJsonValue &value : jsonArray)
    {
        PortItem portItem;
        QJsonObject obj = value.toObject();

        if (!obj.contains("heading") || !obj.contains("use") || !obj.contains("ports"))
        {
            return false;
        }

        portItem.protocol = ProtocolType(obj["heading"].toString());
        portItem.heading = obj["heading"].toString();
        portItem.use = obj["use"].toString();

        const QJsonArray jsonPorts = obj["ports"].toArray();
        for (const QJsonValue &portValue: jsonPorts)
        {
            QString strPort = portValue.toString();
            portItem.ports << strPort.toUInt();
        }

        d->items_ << portItem;
    }

    return true;
}

const PortItem *PortMap::getPortItemByHeading(const QString &heading)
{
    for (const PortItem &portItem : d->items_)
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
    for (const PortItem &portItem : d->items_)
    {
        if (portItem.protocol.isEqual(protocol))
        {
            return &portItem;
        }
    }
    return NULL;
}

int PortMap::getUseIpInd(const ProtocolType &connectionProtocol) const
{
    for (const PortItem &portItem : d->items_)
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

void PortMap::writeToStream(QDataStream &stream)
{
    /*stream << items.count();
    Q_FOREACH(const PortItem &pi, items)
    {
        pi.writeToStream(stream);
    }*/
}

void PortMap::readFromStream(QDataStream &stream)
{
    /*int cnt;
    stream >> cnt;
    for (int i = 0; i < cnt; ++i)
    {
        PortItem pi;
        pi.readFromStream(stream);
        items << pi;
    }*/
}

/*void PortItem::writeToStream(QDataStream &stream) const
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
}*/

} //namespace ApiInfo
