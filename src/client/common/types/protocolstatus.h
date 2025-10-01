#pragma once

#include <QDataStream>
#include <QDebug>
#include <QMetaType>
#include <QString>
#include "protocol.h"

namespace types {

struct ProtocolStatus
{
public:
    enum Status {
        kDisconnected = 0,
        kConnected = 1,
        kUpNext = 2,
        kFailed = 3,
    };

    Protocol protocol;
    int port;
    Status status;
    int timeout;

    ProtocolStatus() : protocol(Protocol::TYPE::WIREGUARD), port(443), status(Status::kDisconnected), timeout(-1) {}
    ProtocolStatus(Protocol proto, int prt, Status st, int sec=-1) : protocol(proto), port(prt), status(st), timeout(sec) {}

    bool operator==(const ProtocolStatus &other) const
    {
        return other.protocol == protocol &&
               other.port == port &&
               other.status == status &&
               other.timeout == timeout;
    }

    bool operator!=(const ProtocolStatus &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const ProtocolStatus &o)
    {
        stream << versionForSerialization_;
        stream << o.protocol << o.port << o.status << o.timeout;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, ProtocolStatus &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.protocol >> o.port >> o.status >> o.timeout;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const ProtocolStatus &ps)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{protocol:" << ps.protocol.toLongString() << "; ";
        dbg << "port:" << ps.port << "; ";
        dbg << "status:" << ps.status << "; ";
        dbg << "timeout:" << ps.timeout << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
