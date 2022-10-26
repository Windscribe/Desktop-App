#include "portmap.h"

#include <QJsonObject>
#include <QMetaType>
#include "utils/ws_assert.h"

const int typeIdPortMap = qRegisterMetaType<types::PortMap>("types::PortMap");

namespace types {

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

        portItem.protocol = Protocol::fromString(obj["heading"].toString());
        if (portItem.protocol == Protocol::UNINITIALIZED)
        {
            return false;
        }

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
    WS_ASSERT(ind >= 0 && ind < d->items_.count());
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

const PortItem *PortMap::getPortItemByProtocolType(const Protocol &protocol) const
{
    for (const PortItem &portItem : d->items_)
    {
        if (portItem.protocol == protocol)
        {
            return &portItem;
        }
    }
    return NULL;
}

int PortMap::getUseIpInd(Protocol connectionProtocol) const
{
    for (const PortItem &portItem : d->items_)
    {
        if (portItem.protocol == connectionProtocol)
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
                WS_ASSERT(false);
                return -1;
            }
        }
    }
    WS_ASSERT(false);
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

void PortMap::removeUnsupportedProtocols(const QList<Protocol> &supportedProtocols)
{
    std::remove_if(d->items_.begin(), d->items_.end(), [supportedProtocols](const PortItem &item) {
            return !supportedProtocols.contains(item.protocol);
        });
}

QDataStream& operator <<(QDataStream& stream, const PortItem& p)
{
    stream << p.versionForSerialization_;
    stream << p.protocol << p.heading << p.use << p.ports;
    return stream;
}

QDataStream& operator >>(QDataStream& stream, PortItem& p)
{
    quint32 version;
    stream >> version;
    if (version > p.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> p.protocol >> p.heading >> p.use >> p.ports;
    return stream;
}

QDataStream& operator <<(QDataStream& stream, const PortMap& p)
{
    stream << p.versionForSerialization_;
    stream << p.d->items_;
    return stream;
}

QDataStream& operator >>(QDataStream& stream, PortMap& p)
{
    quint32 version;
    stream >> version;
    if (version > p.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> p.d->items_;
    return stream;
}

} //namespace types
