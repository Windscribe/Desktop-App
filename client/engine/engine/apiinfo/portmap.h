#ifndef APIINFO_PORTMAP_H
#define APIINFO_PORTMAP_H

#include <QJsonArray>
#include <QSharedDataPointer>
#include <QVector>
#include <qstring.h>
#include "engine/types/protocoltype.h"

namespace apiinfo {

struct PortItem
{
    ProtocolType protocol;
    QString heading;
    QString use;
    QVector<uint> ports;
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


private:
    QSharedDataPointer<PortMapData> d;
};

} //namespace apiinfo


#endif // APIINFO_PORTMAP_H
