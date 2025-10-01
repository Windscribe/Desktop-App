#include "portmap.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include "utils/ws_assert.h"

const int typeIdPortMap = qRegisterMetaType<api_responses::PortMap>("api_responses::PortMap");

namespace api_responses {

PortMap::PortMap(const std::string &json) : d(new PortMapData)
{
    if (json.empty()) {
        return;
    }

    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    if (errCode.error != QJsonParseError::ParseError::NoError) {
        return;
    }

    auto jsonObject = doc.object();
    auto jsonData =  jsonObject["data"].toObject();
    auto jsonArray = jsonData["portmap"].toArray();

    for (const QJsonValue &value : jsonArray)
    {
        PortItem portItem;
        QJsonObject obj = value.toObject();

        if (!obj.contains("heading") || !obj.contains("use") || !obj.contains("ports"))
            return;

        portItem.protocol = types::Protocol::fromString(obj["heading"].toString());
        if (portItem.protocol == types::Protocol::UNINITIALIZED)
            return;

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
    removeUnsupportedProtocols(types::Protocol::supportedProtocols());

    if (d->items_.count() > 0) {
        isValid_ = true;
    }
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

const PortItem *PortMap::getPortItemByProtocolType(const types::Protocol &protocol) const
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

int PortMap::getUseIpInd(types::Protocol connectionProtocol) const
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

void PortMap::removeUnsupportedProtocols(const QList<types::Protocol> &supportedProtocols)
{
    d->items_.erase(std::remove_if(d->items_.begin(), d->items_.end(), [supportedProtocols](const PortItem &item)
    {
        return !supportedProtocols.contains(item.protocol);
    }), d->items_.end());
}

bool PortMap::isValid() const
{
    return isValid_;
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
    stream << p.d->items_ << p.isValid_;
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
    if (version >= 2) {
        stream >> p.isValid_;
    }
    return stream;
}

} //namespace api_responses
