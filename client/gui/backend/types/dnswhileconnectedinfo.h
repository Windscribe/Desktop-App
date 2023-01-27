#ifndef DNSWHILECONNECTEDINFO_H
#define DNSWHILECONNECTEDINFO_H

#include <QString>
#include "ipc/proto/types.pb.h"


class DnsWhileConnectedInfo
{
public:
    enum DNS_WHILE_CONNECTED_TYPE { ROBERT, CUSTOM };

    DnsWhileConnectedInfo();
    explicit DnsWhileConnectedInfo(DNS_WHILE_CONNECTED_TYPE type);
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

    static QString robertText();
    static QString customText();
};

inline bool operator==(const DnsWhileConnectedInfo& lhs, const DnsWhileConnectedInfo& rhs)
{
    return lhs.type() == rhs.type() && lhs.ipAddress() == rhs.ipAddress();
}

inline bool operator!=(const DnsWhileConnectedInfo& lhs, const DnsWhileConnectedInfo& rhs)
{
    return lhs.type() != rhs.type() || lhs.ipAddress() != rhs.ipAddress();
}

#endif // DNSWHILECONNECTEDINFO_H
