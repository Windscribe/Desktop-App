#include "connecteddnsinfo.h"
#include <QObject>
#include "utils/ws_assert.h"

namespace types {

QList<CONNECTED_DNS_TYPE> ConnectedDnsInfo::allAvailableTypes()
{
    QList<CONNECTED_DNS_TYPE> t;
    t << CONNECTED_DNS_TYPE_AUTO << CONNECTED_DNS_TYPE_FORCED << CONNECTED_DNS_TYPE_CUSTOM;
    return t;
}

QString ConnectedDnsInfo::typeToString(const CONNECTED_DNS_TYPE &type)
{
    if (type == CONNECTED_DNS_TYPE_AUTO)
    {
        return QObject::tr("Auto");
    }
    else if (type == CONNECTED_DNS_TYPE_CUSTOM)
    {
        return QObject::tr("Custom");
    }
    else if (type == CONNECTED_DNS_TYPE_FORCED)
    {
        return QObject::tr("Forced");
    }
    else
    {
        WS_ASSERT(false);
        return "";
    }
}

bool ConnectedDnsInfo::operator==(const ConnectedDnsInfo &other) const
{
    return other.type == type &&
           other.upStream1 == upStream1 &&
           other.isSplitDns == isSplitDns &&
           other.upStream2 == upStream2 &&
           other.hostnames == hostnames;
}

bool ConnectedDnsInfo::operator!=(const ConnectedDnsInfo &other) const
{
    return !(*this == other);
}

QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o)
{
    stream << o.versionForSerialization_;
    stream << o.type << o.upStream1 << o.isSplitDns << o.upStream2 << o.hostnames;
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
        stream >> o.type >> o.upStream1;
    else
        stream >> o.type >> o.upStream1 >> o.isSplitDns >> o.upStream2 >> o.hostnames;
    return stream;
}


QDebug operator<<(QDebug dbg, const ConnectedDnsInfo &ds)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{type:" << ds.type << "}";
    return dbg;
}

} // types namespace
