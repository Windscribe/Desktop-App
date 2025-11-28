#pragma once

#include <filesystem>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QVector>

#include "types/enums.h"
#include "utils/ipvalidation.h"
#include "utils/log/categories.h"

namespace types {


struct SplitTunnelingSettings
{
    SplitTunnelingSettings() = default;

    SplitTunnelingSettings(const QJsonObject &json)
    {
        if (json.contains(kJsonActiveProp) && json[kJsonActiveProp].isBool()) {
            active = json[kJsonActiveProp].toBool();
        }

        if (json.contains(kJsonModeProp) && json[kJsonModeProp].isDouble()) {
            mode = SPLIT_TUNNELING_MODE_fromInt(json[kJsonModeProp].toInt());
        }
    }

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

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonActiveProp] = active;
        json[kJsonModeProp] = static_cast<int>(mode);
        if (isForDebugLog) {
            json["modeDesc"] = SPLIT_TUNNELING_MODE_toString(mode);
        }
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

private:
    static const inline QString kJsonActiveProp = "active";
    static const inline QString kJsonModeProp = "mode";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

struct SplitTunnelingNetworkRoute
{
    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    QString name;
    bool active = true;

    SplitTunnelingNetworkRoute() : type(SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP) {}

    SplitTunnelingNetworkRoute(const QJsonObject &json)
    {
        if (json.contains(kJsonTypeProp) && json[kJsonTypeProp].isDouble()) {
            type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_fromInt(json[kJsonTypeProp].toInt());
        }

        if (json.contains(kJsonNameProp) && json[kJsonNameProp].isString()) {
            QString str = json[kJsonNameProp].toString();
            if (IpValidation::isIpCidrOrDomain(str)) {
                name = str;
            }
        }

        if (json.contains(kJsonActiveProp) && json[kJsonActiveProp].isBool()) {
            active = json[kJsonActiveProp].toBool();
        } else {
            active = true;
        }
    }

    bool operator==(const SplitTunnelingNetworkRoute &other) const
    {
        return other.type == type &&
               other.name == name &&
               other.active == active;
    }

    bool operator!=(const SplitTunnelingNetworkRoute &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[kJsonTypeProp] = static_cast<int>(type);
        json[kJsonNameProp] = name;
        json[kJsonActiveProp] = active;
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const SplitTunnelingNetworkRoute &o)
    {
        stream << versionForSerialization_;
        stream << o.type << o.name << o.active;
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

        if (version >= 2) {
            stream >> o.active;
        } else {
            o.active = true;
        }
        return stream;
    }

private:
    static const inline QString kJsonTypeProp = "type";
    static const inline QString kJsonNameProp = "name";
    static const inline QString kJsonActiveProp = "active";

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

struct SplitTunnelingApp
{
    SplitTunnelingApp() = default;
    SplitTunnelingApp& operator=(const SplitTunnelingApp&) = default;

    SplitTunnelingApp(const QJsonObject &json)
    {
        if (json.contains(kJsonTypeProp) && json[kJsonTypeProp].isDouble()) {
            type = SPLIT_TUNNELING_APP_TYPE_fromInt(json[kJsonTypeProp].toInt());
        }

        if (json.contains(kJsonNameProp) && json[kJsonNameProp].isString()) {
            name = json[kJsonNameProp].toString();
        }

        if (json.contains(kJsonFullNameProp) && json[kJsonFullNameProp].isString()) {
            QString path = json[kJsonFullNameProp].toString();
            std::error_code ec;
            if (std::filesystem::exists(path.toStdString(), ec)) {
                fullName = path;
            }
        }

        if (json.contains(kJsonActiveProp) && json[kJsonActiveProp].isBool()) {
            active = json[kJsonActiveProp].toBool();
        } else {
            active = true;
        }

        if (json.contains(kJsonIconProp) && json[kJsonIconProp].isString()) {
            QString path = json[kJsonIconProp].toString();
            std::error_code ec;
            if (std::filesystem::exists(path.toStdString(), ec)) {
                icon = path;
            }
        }
    }

    bool active = true;
    QString fullName;   // path + name
    QString name;
    QString icon;
    SPLIT_TUNNELING_APP_TYPE type = SPLIT_TUNNELING_APP_TYPE_USER;

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

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonActiveProp] = active;
        json[kJsonNameProp] = name;
        json[kJsonTypeProp] = static_cast<int>(type);
        if (isForDebugLog) {
            json["fullNameDesc"] = fullName.isEmpty() ? "empty" : "settled";
            json["iconDesc"] = icon.isEmpty() ? "empty" : "settled";
        } else {
            json[kJsonFullNameProp] = fullName;
            json[kJsonIconProp] = icon;
        }
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

private:
    static const inline QString kJsonActiveProp = "active";
    static const inline QString kJsonFullNameProp = "fullName";
    static const inline QString kJsonNameProp = "name";
    static const inline QString kJsonIconProp = "icon";
    static const inline QString kJsonTypeProp = "type";

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};


struct SplitTunneling
{
    SplitTunneling() = default;

    SplitTunneling(const QJsonObject &json)
    {
        if (json.contains(kJsonSettingsProp) && json[kJsonSettingsProp].isObject()) {
            settings = SplitTunnelingSettings(json[kJsonSettingsProp].toObject());
        }

        if (json.contains(kJsonAppsProp) && json[kJsonAppsProp].isArray()) {
            QJsonArray appsArray = json[kJsonAppsProp].toArray();
            apps.clear();
            apps.reserve(appsArray.size());
            for (const QJsonValue &appValue : appsArray) {
                if (appValue.isObject()) {
                    SplitTunnelingApp app = SplitTunnelingApp(appValue.toObject());
                    if (app.fullName.isEmpty()) {
                        // App not found on this system, skip this entry
                        continue;
                    }
                    apps.append(app);
                }
            }
        }

        if (json.contains(kJsonNetworkRoutesProp) && json[kJsonNetworkRoutesProp].isArray()) {
            QJsonArray networkRoutesArray = json[kJsonNetworkRoutesProp].toArray();
            networkRoutes.clear();
            networkRoutes.reserve(networkRoutesArray.size());
            for (const QJsonValue &routeValue : networkRoutesArray) {
                if (routeValue.isObject()) {
                    SplitTunnelingNetworkRoute route = SplitTunnelingNetworkRoute(routeValue.toObject());
                    if (route.name.isEmpty()) {
                        // Invalid route, skip this entry
                        continue;
                    }
                    networkRoutes.append(route);
                }
            }
        }
    }

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

    void fromIni(const QSettings &s)
    {
        settings.active = s.value(kIniSplitTunnelingEnabledProp, settings.active).toBool();
        settings.mode = SPLIT_TUNNELING_MODE_fromString(s.value(kIniSplitTunnelingModeProp, SPLIT_TUNNELING_MODE_toString(settings.mode)).toString());

        if (s.contains(kIniSplitTunnelingAppsProp)) {
            apps.clear();
            QStringList appsList;
            appsList = s.value(kIniSplitTunnelingAppsProp).toStringList();
            for (auto appPath : appsList) {
                std::error_code ec;
                std::filesystem::path path(appPath.toStdString());
                if (path.empty()) {
                    continue;
                }
                bool exists = std::filesystem::exists(path, ec);
                if (ec || !exists) {
                    qCDebug(LOG_BASIC) << "Skipping non-existent split tunneling app '" << appPath << "'";
                    continue;
                }

                SplitTunnelingApp a;
                a.active = true;
                a.fullName = appPath;
                a.name = appPath;
                a.icon = "";
                a.type = SPLIT_TUNNELING_APP_TYPE_USER;
                apps << a;
            }
        }

        if (s.contains(kIniSplitTunnelingRoutesProp)) {
            networkRoutes.clear();
            QStringList networkRoutesList;
            networkRoutesList = s.value(kIniSplitTunnelingRoutesProp).toStringList();
            for (auto route : networkRoutesList) {
                if (route.isEmpty()) {
                    continue;
                }
                SplitTunnelingNetworkRoute r;
                r.name = route;
                if (IpValidation::isIpCidr(route)) {
                    r.type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
                } else if (IpValidation::isDomain(route)) {
                    r.type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME;
                } else {
                    qCDebug(LOG_BASIC) << "Skipping unrecognized split tunneling route type";
                    continue;
                }
                networkRoutes << r;
            }
        }
    }

    void toIni(QSettings &s) const
    {
        s.setValue(kIniSplitTunnelingEnabledProp, settings.active);
        s.setValue(kIniSplitTunnelingModeProp, SPLIT_TUNNELING_MODE_toString(settings.mode));

        QStringList appsList;
        for (auto app : apps) {
            if (app.active) {
                appsList << app.fullName;
            }
        }
        if (appsList.isEmpty()) {
            s.setValue(kIniSplitTunnelingAppsProp, "");
        } else {
            s.setValue(kIniSplitTunnelingAppsProp, appsList);
        }

        QStringList routesList;
        for (auto route : networkRoutes) {
            if (route.active) {
                routesList << route.name;
            }
        }
        if (routesList.isEmpty()) {
            s.setValue(kIniSplitTunnelingRoutesProp, "");
        } else {
            s.setValue(kIniSplitTunnelingRoutesProp, routesList);
        }
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonSettingsProp] = settings.toJson(isForDebugLog);

        QJsonArray appsArray;
        for (const SplitTunnelingApp& app : apps) {
            appsArray.append(app.toJson(isForDebugLog));
        }
        json[kJsonAppsProp] = appsArray;

        QJsonArray networkRoutesArray;
        for (const SplitTunnelingNetworkRoute& route : networkRoutes) {
            networkRoutesArray.append(route.toJson());
        }
        json[kJsonNetworkRoutesProp] = networkRoutesArray;

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

private:
    static const inline QString kIniSplitTunnelingAppsProp = "SplitTunnelingApps";
    static const inline QString kIniSplitTunnelingRoutesProp = "SplitTunnelingRoutes";
    static const inline QString kIniSplitTunnelingEnabledProp = "SplitTunnelingEnabled";
    static const inline QString kIniSplitTunnelingModeProp = "SplitTunnelingMode";

    static const inline QString kJsonAppsProp = "apps";
    static const inline QString kJsonNetworkRoutesProp = "networkRoutes";
    static const inline QString kJsonSettingsProp = "settings";
    static const inline QString kJsonVersionProp = "version";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};


} // types namespace
