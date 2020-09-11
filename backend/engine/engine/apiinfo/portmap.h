#ifndef APIINFO_PORTMAP_H
#define APIINFO_PORTMAP_H

#include <QJsonArray>
#include <QSharedDataPointer>
#include <QVector>
#include <qstring.h>
#include "engine/types/protocoltype.h"

namespace ApiInfo {

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
    explicit PortMap() { d = new PortMapData; }
    PortMap(const PortMap &other) : d (other.d) { }

    bool initFromJson(const QJsonArray &jsonArray);

    const PortItem* getPortItemByHeading(const QString &heading);
    const PortItem* getPortItemByProtocolType(const ProtocolType &protocol);
    int getUseIpInd(const ProtocolType &connectionProtocol) const;

    void writeToStream(QDataStream &stream);
    void readFromStream(QDataStream &stream);

private:
    QSharedDataPointer<PortMapData> d;
};

} //namespace ApiInfo


#endif // APIINFO_PORTMAP_H
