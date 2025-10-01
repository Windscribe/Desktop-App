#pragma once

#include <QJsonArray>
#include <QSharedDataPointer>
#include <QVector>
#include "types/protocol.h"

namespace api_responses {

struct PortItem
{
    types::Protocol protocol;
    QString heading;
    QString use;
    QVector<uint> ports;

    friend QDataStream& operator <<(QDataStream& stream, const PortItem& p);
    friend QDataStream& operator >>(QDataStream& stream, PortItem& p);

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
    PortMap() : d(new PortMapData) {}
    explicit PortMap(const std::string &json);
    PortMap(const PortMap &other) : d (other.d), isValid_(other.isValid()) {}

    int getPortItemCount() const;
    const PortItem *getPortItemByIndex(int ind) const;
    const PortItem *getPortItemByHeading(const QString &heading) const;
    const PortItem *getPortItemByProtocolType(const types::Protocol &protocol) const;
    int getUseIpInd(types::Protocol connectionProtocol) const;

    QVector<PortItem> &items();
    const QVector<PortItem> &const_items() const;

    void removeUnsupportedProtocols(const QList<types::Protocol> &supportedProtocols);

    bool isValid() const;

    PortMap& operator=(const PortMap&) = default;

    friend QDataStream& operator <<(QDataStream& stream, const PortMap& p);
    friend QDataStream& operator >>(QDataStream& stream, PortMap& p);


private:
    QSharedDataPointer<PortMapData> d;
    bool isValid_ = false;

    static constexpr quint32 versionForSerialization_ = 2;
};

} //namespace api_responses
