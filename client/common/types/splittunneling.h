#pragma once

#include <QVector>
#include <qdebug.h>

#include "types/enums.h"

namespace types {


struct SplitTunnelingSettings
{
    bool active = false;
    SPLIT_TUNNELING_MODE mode = SPLIT_TUNNELING_MODE_EXCLUDE;

    bool operator==(const SplitTunnelingSettings &other) const
    {
        return other.active == active &&
               other.mode == mode;
    }

    bool operator!=(const SplitTunnelingSettings &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const SplitTunnelingSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.active << o.mode;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, SplitTunnelingSettings &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.active >> o.mode;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const SplitTunnelingSettings &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{active:" << s.active << "; ";
        dbg << "mode:" << SPLIT_TUNNELING_MODE_toString(s.mode) << "}";
        return dbg;
    }


private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

struct SplitTunnelingNetworkRoute
{
    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    QString name;

    bool operator==(const SplitTunnelingNetworkRoute &other) const
    {
        return other.type == type &&
               other.name == name;
    }

    bool operator!=(const SplitTunnelingNetworkRoute &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const SplitTunnelingNetworkRoute &o)
    {
        stream << versionForSerialization_;
        stream << o.type << o.name;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, SplitTunnelingNetworkRoute &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.type >> o.name;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const SplitTunnelingNetworkRoute &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{type:" << (int)s.type << "; ";
        dbg << "name:" << s.name << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

struct SplitTunnelingApp
{
    SPLIT_TUNNELING_APP_TYPE type = SPLIT_TUNNELING_APP_TYPE_USER;
    QString name;
    QString fullName;   // path + name
    bool active = false;

    bool operator==(const SplitTunnelingApp &other) const
    {
        return other.type == type &&
               other.name == name &&
               other.fullName == fullName &&
               other.active == active;
    }

    bool operator!=(const SplitTunnelingApp &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const SplitTunnelingApp &o)
    {
        stream << versionForSerialization_;
        stream << o.type << o.name << o.fullName << o.active;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, SplitTunnelingApp &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.type >> o.name >> o.fullName >> o.active;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const SplitTunnelingApp &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{type:" << (int)s.type << "; ";
        dbg << "name:" << s.name << "; ";
        dbg << "fullName:" << (s.fullName.isEmpty() ? "empty" : "settled") << "; ";
        dbg << "active:" << s.active << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};


struct SplitTunneling
{
    SplitTunnelingSettings settings;
    QVector<SplitTunnelingApp> apps;
    QVector<SplitTunnelingNetworkRoute> networkRoutes;

    bool operator==(const SplitTunneling &other) const
    {
        return other.settings == settings &&
               other.apps == apps &&
               other.networkRoutes == networkRoutes;
    }

    bool operator!=(const SplitTunneling &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const SplitTunneling &o)
    {
        stream << versionForSerialization_;
        stream << o.settings << o.apps << o.networkRoutes;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, SplitTunneling &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.settings >> o.apps >> o.networkRoutes;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const SplitTunneling &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{settings:" << s.settings << "; ";
        dbg << "apps:" << s.apps << "; ";
        dbg << "networkRoutes:" << s.networkRoutes << "}";
        return dbg;
    }


private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} // types namespace
