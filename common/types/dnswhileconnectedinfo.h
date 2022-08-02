#ifndef TYPES_DNSWHILECONNECTEDINFO_H
#define TYPES_DNSWHILECONNECTEDINFO_H

#include <QString>
#include <QList>
#include "enums.h"

namespace types {

class DnsWhileConnectedInfo
{
public:    
    DnsWhileConnectedInfo() : type_(DNS_WHILE_CONNECTED_TYPE_ROBERT) {}
    explicit DnsWhileConnectedInfo(ProtoTypes::DnsWhileConnectedInfo protoBufInfo);

    QString toString() const;
    DNS_WHILE_CONNECTED_TYPE type() const;
    void setType(DNS_WHILE_CONNECTED_TYPE type);
    QString ipAddress() const;
    void setIpAddress(const QString &ipAddr);

    ProtoTypes::DnsWhileConnectedInfo toProtobuf();

    static QList<DNS_WHILE_CONNECTED_TYPE> allAvailableTypes();
    static QString typeToString(const DNS_WHILE_CONNECTED_TYPE &type);

private:
    DNS_WHILE_CONNECTED_TYPE type_;
    QString ipAddress_;
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
