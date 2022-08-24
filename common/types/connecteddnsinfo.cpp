#include "connecteddnsinfo.h"
#include <QObject>

namespace types {

QString ConnectedDnsInfo::toString() const
{
    return typeToString(type_);
}

CONNECTED_DNS_TYPE ConnectedDnsInfo::type() const
{
    return type_;
}

void ConnectedDnsInfo::setType(CONNECTED_DNS_TYPE type)
{
    type_ = type;
}

QString ConnectedDnsInfo::ipAddress() const
{
    return ipAddress_;
}

void ConnectedDnsInfo::setIpAddress(const QString &ipAddr)
{
    ipAddress_ = ipAddr;
}

QList<CONNECTED_DNS_TYPE> ConnectedDnsInfo::allAvailableTypes()
{
    QList<CONNECTED_DNS_TYPE> t;
    t << CONNECTED_DNS_TYPE_ROBERT << CONNECTED_DNS_TYPE_CUSTOM;
    return t;
}

QString ConnectedDnsInfo::typeToString(const CONNECTED_DNS_TYPE &type)
{
    if (type == CONNECTED_DNS_TYPE_ROBERT)
    {
        return QObject::tr("R.O.B.E.R.T.");
    }
    else if (type == CONNECTED_DNS_TYPE_CUSTOM)
    {
        return QObject::tr("Custom");
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o)
{
    stream << o.versionForSerialization_;
    stream << o.type_ << o.ipAddress_;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, ConnectedDnsInfo &o)
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


QDebug operator<<(QDebug dbg, const ConnectedDnsInfo &ds)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{type:" << ds.type_ << "}";
    return dbg;
}

} // types namespace
