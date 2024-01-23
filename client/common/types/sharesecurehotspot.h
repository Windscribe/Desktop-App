#pragma once

#include "types/enums.h"
#include "utils/utils.h"

#include <QDataStream>
#include <QDebug>
#include <QJsonObject>


namespace types {

struct ShareSecureHotspot
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kIsEnabledProp = "isEnabled";
        const QString kSSIDProp = "ssid";
        const QString kPasswordProp = "password";
    };

    ShareSecureHotspot() = default;

    ShareSecureHotspot(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kIsEnabledProp) && json[jsonInfo.kIsEnabledProp].isDouble())
            isEnabled = json[jsonInfo.kIsEnabledProp].toBool();

        if (json.contains(jsonInfo.kSSIDProp) && json[jsonInfo.kSSIDProp].isString())
            ssid = Utils::fromBase64(json[jsonInfo.kSSIDProp].toString());

        if (json.contains(jsonInfo.kPasswordProp) && json[jsonInfo.kPasswordProp].isString())
            password = Utils::fromBase64(json[jsonInfo.kPasswordProp].toString());
    }

    bool isEnabled = false;
    QString ssid;
    QString password;
    JsonInfo jsonInfo;  // Include the JsonInfo structure within ShareSecureHotspot

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

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kIsEnabledProp] = isEnabled;
        json[jsonInfo.kSSIDProp] = Utils::toBase64(ssid);
        json[jsonInfo.kPasswordProp] = Utils::toBase64(password);
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

    friend QDebug operator<<(QDebug dbg, const ShareSecureHotspot &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isEnabled:" << s.isEnabled << "; ";
        dbg << "ssid:" << (s.ssid.isEmpty() ? "empty" : "settled") << "; ";
        dbg << "password:" << (s.password.isEmpty() ? "empty" : "settled") << "}";
        return dbg;
    }


private:
    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed

};

} // types namespace
