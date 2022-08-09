#include "dnswhileconnectedinfo.h"
#include <QObject>

namespace types {

QString DnsWhileConnectedInfo::toString() const
{
    return typeToString(type_);
}

DNS_WHILE_CONNECTED_TYPE DnsWhileConnectedInfo::type() const
{
    return type_;
}

void DnsWhileConnectedInfo::setType(DNS_WHILE_CONNECTED_TYPE type)
{
    type_ = type;
}

QString DnsWhileConnectedInfo::ipAddress() const
{
    return ipAddress_;
}

void DnsWhileConnectedInfo::setIpAddress(const QString &ipAddr)
{
    ipAddress_ = ipAddr;
}

QList<DNS_WHILE_CONNECTED_TYPE> DnsWhileConnectedInfo::allAvailableTypes()
{
    QList<DNS_WHILE_CONNECTED_TYPE> t;
    t << DNS_WHILE_CONNECTED_TYPE_ROBERT << DNS_WHILE_CONNECTED_TYPE_CUSTOM;
    return t;
}

QString DnsWhileConnectedInfo::typeToString(const DNS_WHILE_CONNECTED_TYPE &type)
{
    return DNS_WHILE_CONNECTED_TYPE_toString(type);
}

QDataStream& operator <<(QDataStream &stream, const DnsWhileConnectedInfo &o)
{
    stream << o.versionForSerialization_;
    stream << o.type_ << o.ipAddress_;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, DnsWhileConnectedInfo &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.type_ >> o.ipAddress_;
    return stream;
}


QDebug operator<<(QDebug dbg, const DnsWhileConnectedInfo &ds)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{type:" << DNS_WHILE_CONNECTED_TYPE_toString(ds.type_) << "}";
    return dbg;
}

} // types namespace
