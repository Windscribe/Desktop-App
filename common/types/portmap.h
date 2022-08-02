#ifndef TYPES_PORTMAP_H
#define TYPES_PORTMAP_H

#include <QJsonArray>
#include <QSharedDataPointer>
#include <QVector>
#include <qstring.h>
#include "protocoltype.h"

namespace types {

struct PortItem
{
    ProtocolType protocol;
    QString heading;
    QString use;
    QVector<uint> ports;

    friend QDataStream& operator <<(QDataStream& stream, const PortItem& p)
    {
        stream << versionForSerialization_;
        stream << p.protocol << p.heading << p.use << p.ports;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, PortItem& p)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        stream >> p.protocol >> p.heading >> p.use >> p.ports;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;
};

class PortMapData : public QSharedData
{
public:
    PortMapData() {}

    PortMapData(const PortMapData &other)
        : QSharedData(other),
          items_(other.items_) {}

    ~PortMapData() {}

    QVector<PortItem> items_;
};

// implicitly shared class PortMap
class PortMap
{
public:
    explicit PortMap() : d(new PortMapData) {}
    PortMap(const PortMap &other) : d (other.d) {}

    bool initFromJson(const QJsonArray &jsonArray);

    int getPortItemCount() const;
    const PortItem *getPortItemByIndex(int ind) const;
    const PortItem *getPortItemByHeading(const QString &heading) const;
    const PortItem *getPortItemByProtocolType(const ProtocolType &protocol) const;
    int getUseIpInd(const ProtocolType &connectionProtocol) const;

    QVector<PortItem> &items();
    const QVector<PortItem> &const_items() const;

    PortMap& operator=(const PortMap&) = default;

    friend QDataStream& operator <<(QDataStream& stream, const PortMap& p)
    {
        stream << versionForSerialization_;
        stream << p.d->items_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, PortMap& p)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        stream >> p.d->items_;
        return stream;
    }


private:
    QSharedDataPointer<PortMapData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types


#endif // TYPES_PORTMAP_H
