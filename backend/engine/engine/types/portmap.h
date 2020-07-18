#ifndef PORTMAP_H
#define PORTMAP_H

#include <QVector>
#include <qstring.h>
#include "Engine/Types/protocoltype.h"

struct PortItem
{
    ProtocolType protocol;
    QString heading;
    QString use;
    QVector<uint> ports;
    uint legacy_port;

    void writeToStream(QDataStream &stream) const;
    void readFromStream(QDataStream &stream);
};

struct PortMap
{
    QVector<PortItem> items;

    const PortItem* getPortItemByHeading(const QString &heading);
    const PortItem* getPortItemByProtocolType(const ProtocolType &protocol);
    int getUseIpInd(const ProtocolType &connectionProtocol);
    uint getLegacyPort(const ProtocolType &connectionProtocol);

    void writeToStream(QDataStream &stream);
    void readFromStream(QDataStream &stream);
};

#endif // PORTMAP_H
