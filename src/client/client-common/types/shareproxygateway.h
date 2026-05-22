#pragma once

#include "types/enums.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"

#include <QDebug>
#include <QJsonObject>
#include <QSettings>

namespace types {

struct ShareProxyGateway
{
    ShareProxyGateway() = default;

    ShareProxyGateway(const QJsonObject &json)
    {
        if (json.contains(kJsonIsEnabledProp) && json[kJsonIsEnabledProp].isBool()) {
            isEnabled = json[kJsonIsEnabledProp].toBool();
        }

        if (json.contains(kJsonProxySharingModeProp) && json[kJsonProxySharingModeProp].isDouble()) {
            proxySharingMode = PROXY_SHARING_TYPE_fromInt(json[kJsonProxySharingModeProp].toInt());
        }

        if (json.contains(kJsonWhileConnectedProp) && json[kJsonWhileConnectedProp].isBool()) {
            whileConnected = json[kJsonWhileConnectedProp].toBool();
        }

        if (json.contains(kJsonPortProp) && json[kJsonPortProp].isDouble()) {
            uint p = static_cast<uint>(json[kJsonPortProp].toInt());
            if (p < 65536) {
                port = p;
            }
        }

        // A pre-auth JSON that had isEnabled=true keeps requireAuth=false
        // to avoid breaking an existing setup.
        if (json.contains(kJsonRequireAuthProp) && json[kJsonRequireAuthProp].isBool()) {
            requireAuth = json[kJsonRequireAuthProp].toBool();
        } else if (isEnabled) {
            requireAuth = false;
            upgradedFromV3 = true;
        }

        if (json.contains(kJsonUsernameProp) && json[kJsonUsernameProp].isString()) {
            username = json[kJsonUsernameProp].toString();
        }

        if (json.contains(kJsonPasswordProp) && json[kJsonPasswordProp].isString()) {
            password = json[kJsonPasswordProp].toString();
        }
    }

    bool isEnabled = false;
    PROXY_SHARING_TYPE proxySharingMode = PROXY_SHARING_HTTP;
    uint port = 0;
    bool whileConnected = false;
    bool requireAuth = true;
    QString username;
    QString password;

    // Set true by the load paths when migrating a pre-auth record with isEnabled=true. Used by the frontend to
    // display a one-time warning.
    bool upgradedFromV3 = false;

    bool operator==(const ShareProxyGateway &other) const
    {
        return other.isEnabled == isEnabled &&
               other.proxySharingMode == proxySharingMode &&
               other.port == port &&
               other.whileConnected == whileConnected &&
               other.requireAuth == requireAuth &&
               other.username == username &&
               other.password == password &&
               other.upgradedFromV3 == upgradedFromV3;
    }

    bool operator!=(const ShareProxyGateway &other) const
    {
        return !(*this == other);
    }

    // True iff two records would produce an identical running server. Excludes upgradedFromV3 (transient UI flag).
    bool functionallyEqual(const ShareProxyGateway &other) const
    {
        return other.isEnabled == isEnabled &&
               other.proxySharingMode == proxySharingMode &&
               other.port == port &&
               other.whileConnected == whileConnected &&
               other.requireAuth == requireAuth &&
               other.username == username &&
               other.password == password;
    }

    void fromIni(const QSettings &settings)
    {
        isEnabled = settings.value(kIniIsEnabledProp, isEnabled).toBool();
        proxySharingMode = PROXY_SHARING_TYPE_fromString(settings.value(kIniProxySharingModeProp, PROXY_SHARING_TYPE_toString(proxySharingMode)).toString());
        uint p = settings.value(kIniPortProp, port).toUInt();
        if (p < 65536) {
            port = p;
        }
        whileConnected = settings.value(kIniWhileConnectedProp, whileConnected).toBool();
        // A pre-auth INI that had the gateway enabled keeps requireAuth=false
        // to avoid breaking an existing setup.
        if (settings.contains(kIniRequireAuthProp)) {
            requireAuth = settings.value(kIniRequireAuthProp, requireAuth).toBool();
        } else if (isEnabled) {
            requireAuth = false;
            upgradedFromV3 = true;
        }
        username = settings.value(kIniUsernameProp, username).toString();
        password = settings.value(kIniPasswordProp, password).toString();
    }

    void toIni(QSettings &settings) const
    {
        settings.setValue(kIniIsEnabledProp, isEnabled);
        settings.setValue(kIniProxySharingModeProp, PROXY_SHARING_TYPE_toString(proxySharingMode));
        settings.setValue(kIniPortProp, port);
        settings.setValue(kIniWhileConnectedProp, whileConnected);
        settings.setValue(kIniRequireAuthProp, requireAuth);
        settings.setValue(kIniUsernameProp, username);
        settings.setValue(kIniPasswordProp, password);
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonIsEnabledProp] = isEnabled;
        json[kJsonProxySharingModeProp] = static_cast<int>(proxySharingMode);
        json[kJsonPortProp] = static_cast<int>(port);
        json[kJsonWhileConnectedProp] = whileConnected;
        json[kJsonRequireAuthProp] = requireAuth;
        if (isForDebugLog) {
            json["proxySharingModeDesc"] = PROXY_SHARING_TYPE_toString(proxySharingMode);
            json["usernameDesc"] = username.isEmpty() ? "empty" : "set";
            json["passwordDesc"] = password.isEmpty() ? "empty" : "set";
        } else {
            json[kJsonUsernameProp] = username;
            json[kJsonPasswordProp] = password;
        }
        return json;
    }

    void validate()
    {
        proxySharingMode = PROXY_SHARING_TYPE_fromInt(static_cast<int>(proxySharingMode));
        if (port >= 65536) {
            qCWarning(LOG_BASIC) << "ShareProxyGateway: invalid port, resetting";
            port = 0;
            isEnabled = false;
        }
        constexpr int kMaxCredentialLen = 255;
        if (username.size() > kMaxCredentialLen || username.contains(QChar(0)) ||
            password.size() > kMaxCredentialLen || password.contains(QChar(0))) {
            qCWarning(LOG_BASIC) << "ShareProxyGateway: credentials out of bounds, disabling";
            username.clear();
            password.clear();
            isEnabled = false;
        }
    }

    friend QDataStream& operator <<(QDataStream &stream, const ShareProxyGateway &o)
    {
        stream << versionForSerialization_;
        stream << o.isEnabled << o.proxySharingMode << o.port << o.whileConnected;
        stream << o.requireAuth << o.username << o.password;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, ShareProxyGateway &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isEnabled >> o.proxySharingMode;
        if (version >= 2) {
            stream >> o.port;
        }
        if (version >= 3) {
            stream >> o.whileConnected;
        }
        if (version >= 4) {
            stream >> o.requireAuth >> o.username >> o.password;
        } else if (o.isEnabled) {
            // Pre-v4 users who already had the proxy gateway enabled shouldn't have their working setup broken by a
            // new auth requirement; carry their open-proxy state forward. Pre-v4 users who had it disabled fall through
            // to the struct default (requireAuth = true) so they get the safe default the first time they turn it on.
            o.requireAuth = false;
            o.upgradedFromV3 = true;
        }
        return stream;
    }

private:
    static const inline QString kIniIsEnabledProp = "ShareProxyGatewayEnabled";
    static const inline QString kIniProxySharingModeProp = "ShareProxyGatewayMode";
    static const inline QString kIniPortProp = "ShareProxyGatewayPort";
    static const inline QString kIniWhileConnectedProp = "ShareProxyGatewayWhileConnected";
    static const inline QString kIniRequireAuthProp = "ShareProxyGatewayRequireAuth";
    static const inline QString kIniUsernameProp = "ShareProxyGatewayUsername";
    static const inline QString kIniPasswordProp = "ShareProxyGatewayPassword";

    static const inline QString kJsonIsEnabledProp = "isEnabled";
    static const inline QString kJsonProxySharingModeProp = "proxySharingMode";
    static const inline QString kJsonPortProp = "port";
    static const inline QString kJsonWhileConnectedProp = "whileConnected";
    static const inline QString kJsonRequireAuthProp = "requireAuth";
    static const inline QString kJsonUsernameProp = "username";
    static const inline QString kJsonPasswordProp = "password";

    static constexpr quint32 versionForSerialization_ = 4;  // should increment the version if the data format is changed
};

} // types namespace

Q_DECLARE_METATYPE(types::ShareProxyGateway)
