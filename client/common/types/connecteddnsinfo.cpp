#include "connecteddnsinfo.h"
#include <QObject>
#include "utils/ws_assert.h"

namespace types {

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
        WS_ASSERT(false);
        return "";
    }
}

bool ConnectedDnsInfo::operator==(const ConnectedDnsInfo &other) const
{
    return other.type_ == type_ &&
           other.upStream1_ == upStream1_ &&
           other.isSplitDns_ == isSplitDns_ &&
           other.upStream2_ == upStream2_ &&
           other.hostnames_ == hostnames_;
}

bool ConnectedDnsInfo::operator!=(const ConnectedDnsInfo &other) const
{
    return !(*this == other);
}

QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o)
{
    stream << o.versionForSerialization_;
    stream << o.type_ << o.upStream1_ << o.isSplitDns_ << o.upStream2_ << o.hostnames_;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, ConnectedDnsInfo &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_) {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }

    if (version == 1)
        stream >> o.type_ >> o.upStream1_;
    else
        stream >> o.type_ >> o.upStream1_ >> o.isSplitDns_ >> o.upStream2_ >> o.hostnames_;

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
