#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

#include "types/enums.h"

namespace types {


struct SplitTunnelingSettings
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kActiveProp = "active";
        const QString kModeProp = "mode";
    };

    SplitTunnelingSettings() = default;

    SplitTunnelingSettings(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kActiveProp) && json[jsonInfo.kActiveProp].isBool())
            active = json[jsonInfo.kActiveProp].toBool();

        if (json.contains(jsonInfo.kModeProp) && json[jsonInfo.kModeProp].isDouble())
            mode = static_cast<SPLIT_TUNNELING_MODE>(json[jsonInfo.kModeProp].toInt());
    }

    bool active = false;
    SPLIT_TUNNELING_MODE mode = SPLIT_TUNNELING_MODE_EXCLUDE;
    JsonInfo jsonInfo;

    bool operator==(const SplitTunnelingSettings &other) const
    {
        return other.active == active &&
               other.mode == mode;
    }

    bool operator!=(const SplitTunnelingSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kActiveProp] = active;
        json[jsonInfo.kModeProp] = static_cast<int>(mode);
        return json;
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
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kTypeProp = "type";
        const QString kNameProp = "name";
    };

    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    QString name;
    JsonInfo jsonInfo;

    SplitTunnelingNetworkRoute() : type(SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP) {}

    SplitTunnelingNetworkRoute(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kTypeProp) && json[jsonInfo.kTypeProp].isDouble())
            type = static_cast<SPLIT_TUNNELING_NETWORK_ROUTE_TYPE>(json[jsonInfo.kTypeProp].toInt());

        if (json.contains(jsonInfo.kNameProp) && json[jsonInfo.kNameProp].isString())
            name = json[jsonInfo.kNameProp].toString();
    }

    bool operator==(const SplitTunnelingNetworkRoute &other) const
    {
        return other.type == type &&
               other.name == name;
    }

    bool operator!=(const SplitTunnelingNetworkRoute &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kTypeProp] = static_cast<int>(type);
        json[jsonInfo.kNameProp] = name;
        return json;
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
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kActiveProp = "active";
        const QString kFullNameProp = "fullName";
        const QString kNameProp = "name";
        const QString kIconProp = "icon";
        const QString kTypeProp = "type";
    };

    SplitTunnelingApp() = default;
    SplitTunnelingApp& operator=(const SplitTunnelingApp&) = default;

    SplitTunnelingApp(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kTypeProp) && json[jsonInfo.kTypeProp].isDouble())
            type = static_cast<SPLIT_TUNNELING_APP_TYPE>(json[jsonInfo.kTypeProp].toInt());

        if (json.contains(jsonInfo.kNameProp) && json[jsonInfo.kNameProp].isString())
            name = json[jsonInfo.kNameProp].toString();

        if (json.contains(jsonInfo.kFullNameProp) && json[jsonInfo.kFullNameProp].isString())
            fullName = json[jsonInfo.kFullNameProp].toString();

        if (json.contains(jsonInfo.kActiveProp) && json[jsonInfo.kActiveProp].isBool())
            active = json[jsonInfo.kActiveProp].toBool();

        if (json.contains(jsonInfo.kIconProp) && json[jsonInfo.kIconProp].isString())
            icon = json[jsonInfo.kIconProp].toString();
    }

    bool active = false;
    QString fullName;   // path + name
    QString name;
    QString icon;
    SPLIT_TUNNELING_APP_TYPE type = SPLIT_TUNNELING_APP_TYPE_USER;
    JsonInfo jsonInfo;

    bool operator==(const SplitTunnelingApp &other) const
    {
        return other.type == type &&
               other.name == name &&
               other.fullName == fullName &&
               other.active == active &&
               other.icon == icon;
    }

    bool operator!=(const SplitTunnelingApp &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kActiveProp] = active;
        json[jsonInfo.kFullNameProp] = fullName;
        json[jsonInfo.kNameProp] = name;
        json[jsonInfo.kTypeProp] = static_cast<int>(type);
        json[jsonInfo.kIconProp] = icon;
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const SplitTunnelingApp &o)
    {
        stream << versionForSerialization_;
        stream << o.type << o.name << o.fullName << o.active << o.icon;
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

        if (version >= 2) {
            stream >> o.icon;
        }
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const SplitTunnelingApp &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{type:" << (int)s.type << "; ";
        dbg << "name:" << s.name << "; ";
        dbg << "fullName:" << (s.fullName.isEmpty() ? "empty" : "settled") << "; ";
        dbg << "active:" << s.active << "; ";
        dbg << "icon:" << (s.icon.isEmpty() ? "empty" : "settled") << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};


struct SplitTunneling
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kAppsProp = "apps";
        const QString kNetworkRoutesProp = "networkRoutes";
        const QString kSettingsProp = "settings";
        const QString kVersionProp = "version";
    };

    SplitTunneling() = default;

    SplitTunneling(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kSettingsProp) && json[jsonInfo.kSettingsProp].isObject())
            settings = SplitTunnelingSettings(json[jsonInfo.kSettingsProp].toObject());

        if (json.contains(jsonInfo.kAppsProp) && json[jsonInfo.kAppsProp].isArray())
        {
            QJsonArray appsArray = json[jsonInfo.kAppsProp].toArray();
            apps.clear();
            apps.reserve(appsArray.size());
            for (const QJsonValue &appValue : appsArray)
            {
                if (appValue.isObject())
                {
                    apps.append(SplitTunnelingApp(appValue.toObject()));
                }
            }
        }

        if (json.contains(jsonInfo.kNetworkRoutesProp) && json[jsonInfo.kNetworkRoutesProp].isArray())
        {
            QJsonArray networkRoutesArray = json[jsonInfo.kNetworkRoutesProp].toArray();
            networkRoutes.clear();
            networkRoutes.reserve(networkRoutesArray.size());
            for (const QJsonValue &routeValue : networkRoutesArray)
            {
                if (routeValue.isObject())
                {
                    networkRoutes.append(SplitTunnelingNetworkRoute(routeValue.toObject()));
                }
            }
        }
    }

    SplitTunnelingSettings settings;
    QVector<SplitTunnelingApp> apps;
    QVector<SplitTunnelingNetworkRoute> networkRoutes;
    JsonInfo jsonInfo;

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

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kSettingsProp] = settings.toJson();

        QJsonArray appsArray;
        for (const SplitTunnelingApp& app : apps)
        {
            appsArray.append(app.toJson());
        }
        json[jsonInfo.kAppsProp] = appsArray;

        QJsonArray networkRoutesArray;
        for (const SplitTunnelingNetworkRoute& route : networkRoutes)
        {
            networkRoutesArray.append(route.toJson());
        }
        json[jsonInfo.kNetworkRoutesProp] = networkRoutesArray;

        json[jsonInfo.kVersionProp] = static_cast<int>(versionForSerialization_);

        return json;
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
