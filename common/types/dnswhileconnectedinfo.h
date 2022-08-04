#ifndef TYPES_DNSWHILECONNECTEDINFO_H
#define TYPES_DNSWHILECONNECTEDINFO_H

#include <QString>
#include <QList>
#include <QDataStream>
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

    friend QDataStream& operator <<(QDataStream &stream, const DnsWhileConnectedInfo &o)
    {
        stream << versionForSerialization_;
        stream << o.type_ << o.ipAddress_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, DnsWhileConnectedInfo &o)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version > versionForSerialization_)
        {
            return stream;
        }
        stream >> o.type_ >> o.ipAddress_;
        return stream;
    }

private:
    DNS_WHILE_CONNECTED_TYPE type_;
    QString ipAddress_;
    static constexpr quint32 versionForSerialization_ = 1;
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
