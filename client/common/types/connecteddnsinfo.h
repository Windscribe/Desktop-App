#pragma once

#include <QString>
#include <QList>
#include <QDataStream>
#include <QJsonObject>
#include "enums.h"

namespace types {

struct ConnectedDnsInfo
{
    CONNECTED_DNS_TYPE type = CONNECTED_DNS_TYPE_ROBERT;
    QString upStream1;
    bool isSplitDns = false;
    QString upStream2;
    QStringList hostnames;

    static QList<CONNECTED_DNS_TYPE> allAvailableTypes();
    static QString typeToString(const CONNECTED_DNS_TYPE &type);

    friend QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectedDnsInfo &o);

    friend QDebug operator<<(QDebug dbg, const ConnectedDnsInfo &ds);

    bool operator==(const ConnectedDnsInfo &other) const;
    bool operator!=(const ConnectedDnsInfo &other) const;

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

} // types namespace
