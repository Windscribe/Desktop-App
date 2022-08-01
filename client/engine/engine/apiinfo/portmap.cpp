#include "portmap.h"

#include <QJsonObject>
#include <QMetaType>

const int typeIdPortMap = qRegisterMetaType<apiinfo::PortMap>("apiinfo::PortMap");

namespace apiinfo {

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

int PortMap::getPortItemCount() const
{
    return d->items_.count();
}

const PortItem *PortMap::getPortItemByIndex(int ind) const
{
    Q_ASSERT(ind >= 0 && ind < d->items_.count());
    if (ind >= 0 && ind < d->items_.count())
    {
        return &d->items_[ind];
    }
    else
    {
        return NULL;
    }
}

const PortItem *PortMap::getPortItemByHeading(const QString &heading) const
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

const PortItem *PortMap::getPortItemByProtocolType(const ProtocolType &protocol) const
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

QVector<PortItem> &PortMap::items()
{
    return d->items_;
}

const QVector<PortItem> &PortMap::const_items() const
{
    return d->items_;
}

} //namespace apiinfo
