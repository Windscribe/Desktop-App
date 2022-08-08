#ifndef TYPES_DNSWHILECONNECTEDINFO_H
#define TYPES_DNSWHILECONNECTEDINFO_H

#include <QString>
#include <QList>
#include <QDataStream>
#include <QJsonObject>
#include "enums.h"

namespace types {

class DnsWhileConnectedInfo
{
public:    
    DnsWhileConnectedInfo() : type_(DNS_WHILE_CONNECTED_TYPE_ROBERT) {}

    QString toString() const;
    DNS_WHILE_CONNECTED_TYPE type() const;
    void setType(DNS_WHILE_CONNECTED_TYPE type);
    QString ipAddress() const;
    void setIpAddress(const QString &ipAddr);

    static QList<DNS_WHILE_CONNECTED_TYPE> allAvailableTypes();
    static QString typeToString(const DNS_WHILE_CONNECTED_TYPE &type);

    friend QDataStream& operator <<(QDataStream &stream, const DnsWhileConnectedInfo &o);
    friend QDataStream& operator >>(QDataStream &stream, DnsWhileConnectedInfo &o);

    friend QDebug operator<<(QDebug dbg, const DnsWhileConnectedInfo &ds);

private:
    DNS_WHILE_CONNECTED_TYPE type_;
    QString ipAddress_;

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

inline bool operator==(const DnsWhileConnectedInfo& lhs, const DnsWhileConnectedInfo& rhs)
{
    return lhs.type() == rhs.type() && lhs.ipAddress() == rhs.ipAddress();
}

inline bool operator!=(const DnsWhileConnectedInfo& lhs, const DnsWhileConnectedInfo& rhs)
{
    return lhs.type() != rhs.type() || lhs.ipAddress() != rhs.ipAddress();
}



} // types namespace

#endif // TYPES_DNSWHILECONNECTEDINFO_H
