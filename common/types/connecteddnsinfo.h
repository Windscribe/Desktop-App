#ifndef TYPES_CONNECTEDDNSINFO_H
#define TYPES_CONNECTEDDNSINFO_H

#include <QString>
#include <QList>
#include <QDataStream>
#include <QJsonObject>
#include "enums.h"

namespace types {

class ConnectedDnsInfo
{
public:    
    ConnectedDnsInfo() : type_(CONNECTED_DNS_TYPE_ROBERT) {}

    QString toString() const;
    CONNECTED_DNS_TYPE type() const;
    void setType(CONNECTED_DNS_TYPE type);
    QString ipAddress() const;
    void setIpAddress(const QString &ipAddr);

    static QList<CONNECTED_DNS_TYPE> allAvailableTypes();
    static QString typeToString(const CONNECTED_DNS_TYPE &type);

    friend QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectedDnsInfo &o);

    friend QDebug operator<<(QDebug dbg, const ConnectedDnsInfo &ds);

private:
    CONNECTED_DNS_TYPE type_;
    QString ipAddress_;

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

inline bool operator==(const ConnectedDnsInfo& lhs, const ConnectedDnsInfo& rhs)
{
    return lhs.type() == rhs.type() && lhs.ipAddress() == rhs.ipAddress();
}

inline bool operator!=(const ConnectedDnsInfo& lhs, const ConnectedDnsInfo& rhs)
{
    return lhs.type() != rhs.type() || lhs.ipAddress() != rhs.ipAddress();
}



} // types namespace

#endif // TYPES_CONNECTEDDNSINFO_H
