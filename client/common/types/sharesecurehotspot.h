#pragma once

#include "types/enums.h"
#include "utils/utils.h"

#include <QDataStream>
#include <QDebug>
#include <QJsonObject>


namespace types {

struct ShareSecureHotspot
{
    ShareSecureHotspot() = default;

    ShareSecureHotspot(const QJsonObject &json)
    {
        if (json.contains(kJsonIsEnabledProp) && json[kJsonIsEnabledProp].isBool())
            isEnabled = json[kJsonIsEnabledProp].toBool();

        if (json.contains(kJsonSSIDProp) && json[kJsonSSIDProp].isString())
            ssid = Utils::fromBase64(json[kJsonSSIDProp].toString());

        if (json.contains(kJsonPasswordProp) && json[kJsonPasswordProp].isString())
            password = Utils::fromBase64(json[kJsonPasswordProp].toString());
    }

    bool isEnabled = false;
    QString ssid;
    QString password;

    bool operator==(const ShareSecureHotspot &other) const
    {
        return other.isEnabled == isEnabled &&
               other.ssid == ssid &&
               other.password == password;
    }

    bool operator!=(const ShareSecureHotspot &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonIsEnabledProp] = isEnabled;
        if (isForDebugLog) {
            json["ssidDesc"] = ssid.isEmpty() ? "empty" : "settled";
            json["passwordDesc"] = password.isEmpty() ? "empty" : "settled";
        } else {
            json[kJsonSSIDProp] = Utils::toBase64(ssid);
            json[kJsonPasswordProp] = Utils::toBase64(password);
        }
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const ShareSecureHotspot &o)
    {
        stream << versionForSerialization_;
        stream << o.isEnabled << o.password << o.ssid;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, ShareSecureHotspot &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isEnabled >> o.password;

        if (version >= 2) {
            stream >> o.ssid;
        }
        return stream;
    }

private:
    static const inline QString kJsonIsEnabledProp = "isEnabled";
    static const inline QString kJsonSSIDProp = "ssid";
    static const inline QString kJsonPasswordProp = "password";

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

} // types namespace
