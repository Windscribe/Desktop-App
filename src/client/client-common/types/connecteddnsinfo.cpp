#include "connecteddnsinfo.h"
#include <QObject>
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

namespace types {

ConnectedDnsInfo::ConnectedDnsInfo(const QJsonObject &json)
{
    if (json.contains(kJsonTypeProp) && json[kJsonTypeProp].isDouble()) {
        type = enumFromInt<CONNECTED_DNS_TYPE>(json[kJsonTypeProp].toInt());
    }

    if (json.contains(kJsonUpStream1Prop) && json[kJsonUpStream1Prop].isString()) {
        upStream1 = json[kJsonUpStream1Prop].toString();
    }

    if (json.contains(kJsonIsSplitDnsProp) && json[kJsonIsSplitDnsProp].isBool()) {
        isSplitDns = json[kJsonIsSplitDnsProp].toBool();
    }

    if (json.contains(kJsonUpStream2Prop) && json[kJsonUpStream2Prop].isString()) {
        upStream2 = json[kJsonUpStream2Prop].toString();
    }

    if (json.contains(kJsonHostnamesProp) && json[kJsonHostnamesProp].isArray()) {
        QJsonArray hostnamesArray = json[kJsonHostnamesProp].toArray();
        hostnames.clear();
        for (const QJsonValue &hostnameValue : hostnamesArray) {
            if (hostnameValue.isString()) {
                hostnames.append(hostnameValue.toString());
            }
        }
    }

    if (json.contains(kJsonControldApiKeyProp) && json[kJsonControldApiKeyProp].isString()) {
        controldApiKey = Utils::fromBase64(json[kJsonControldApiKeyProp].toString());
    }
}

QList<CONNECTED_DNS_TYPE> ConnectedDnsInfo::allAvailableTypes()
{
    QList<CONNECTED_DNS_TYPE> t;
    t << CONNECTED_DNS_TYPE_AUTO << CONNECTED_DNS_TYPE_CONTROLD << CONNECTED_DNS_TYPE_CUSTOM;
    return t;
}

bool ConnectedDnsInfo::isCustomIPv4Address() const
{
    return type == CONNECTED_DNS_TYPE_CUSTOM && NetworkingValidation::isIp(upStream1) && isSplitDns == false;
}

void ConnectedDnsInfo::normalize()
{
    // A custom DNS upstream given as the wildcard listen address is reached on loopback, so we
    // store the loopback equivalent as the actual upstream. Control D resolvers are not listen
    // addresses, so only custom upstreams are touched.
    if (type != CONNECTED_DNS_TYPE_CUSTOM) {
        return;
    }
    const auto toLoopback = [](const QString &ip) {
        if (NetworkingValidation::isUnspecifiedIp(ip)) {
            return QString(NetworkingValidation::isIpv6(ip) ? "::1" : "127.0.0.1");
        }
        return ip;
    };
    upStream1 = toLoopback(upStream1);
    if (isSplitDns) {
        upStream2 = toLoopback(upStream2);
    }
}

QJsonObject ConnectedDnsInfo::toJson(bool isForDebugLog) const
{
    QJsonObject json;
    json[kJsonTypeProp] = static_cast<int>(type);
    json[kJsonUpStream1Prop] = upStream1;
    json[kJsonIsSplitDnsProp] = isSplitDns;
    json[kJsonUpStream2Prop] = upStream2;

    QJsonArray hostnamesArray;
    for (const QString& hostname : hostnames) {
        hostnamesArray.append(hostname);
    }
    json[kJsonHostnamesProp] = hostnamesArray;

    if (isForDebugLog) {
        json[kJsonControldApiKeyProp] = controldApiKey.isEmpty() ? "empty" : "set";
    } else {
        json[kJsonControldApiKeyProp] = Utils::toBase64(controldApiKey);
    }

    return json;
}

#ifdef CLI_ONLY
void ConnectedDnsInfo::fromIni(const QSettings &settings)
{
    type = enumFromString<CONNECTED_DNS_TYPE>(settings.value(kIniTypeProp, "Auto").toString());

    // If using CLI-only, this type is not supported, change it to custom instead.
    if (type == CONNECTED_DNS_TYPE_CONTROLD) {
        type = CONNECTED_DNS_TYPE_CUSTOM;
    }

    upStream1 = settings.value(kIniUpStream1Prop).toString();
    isSplitDns = settings.value(kIniIsSplitDnsProp, false).toBool();
    upStream2 = settings.value(kIniUpStream2Prop).toString();
    hostnames = settings.value(kIniHostnamesProp).toStringList();
}

void ConnectedDnsInfo::toIni(QSettings &settings) const
{
    // CONNECTED_DNS_TYPE_CONTROLD is not supported for INIs, change it to custom.
    CONNECTED_DNS_TYPE typeToWrite = type;
    if (typeToWrite == CONNECTED_DNS_TYPE_CONTROLD) {
        typeToWrite = CONNECTED_DNS_TYPE_CUSTOM;
    }

    settings.setValue(kIniTypeProp, enumToString(typeToWrite));
    settings.setValue(kIniUpStream1Prop, upStream1);
    settings.setValue(kIniIsSplitDnsProp, isSplitDns);
    settings.setValue(kIniUpStream2Prop, upStream2);
    if (hostnames.isEmpty()) {
        settings.setValue(kIniHostnamesProp, "");
    } else {
        settings.setValue(kIniHostnamesProp, hostnames);
    }
}
#endif

void ConnectedDnsInfo::validate()
{
    type = enumFromInt<CONNECTED_DNS_TYPE>(static_cast<int>(type));
    if (type == CONNECTED_DNS_TYPE_FORCED) {
        type = CONNECTED_DNS_TYPE_AUTO;
    }

    if (!upStream1.isEmpty() && !NetworkingValidation::isCtrldCorrectAddress(upStream1)) {
        qCWarning(LOG_BASIC) << "ConnectedDnsInfo: invalid upStream1, falling back to default";
        upStream1.clear();
        type = CONNECTED_DNS_TYPE_AUTO;
    }
    if (!upStream2.isEmpty() && !NetworkingValidation::isCtrldCorrectAddress(upStream2)) {
        qCWarning(LOG_BASIC) << "ConnectedDnsInfo: invalid upStream2, clearing";
        upStream2.clear();
    }

    constexpr int kMaxApiKeyLen = 255;
    if (controldApiKey.size() > kMaxApiKeyLen || controldApiKey.contains(QChar(0))) {
        qCWarning(LOG_BASIC) << "ConnectedDnsInfo: controldApiKey out of bounds, clearing";
        controldApiKey.clear();
        if (type == CONNECTED_DNS_TYPE_CONTROLD) {
            type = CONNECTED_DNS_TYPE_AUTO;
        }
    }

    normalize();

    constexpr int kMaxHostnames = 1024;
    if (hostnames.size() > kMaxHostnames) {
        qCWarning(LOG_BASIC) << "ConnectedDnsInfo: hostnames over cap, truncating";
        hostnames = hostnames.mid(0, kMaxHostnames);
    }
    QStringList filtered;
    filtered.reserve(hostnames.size());
    for (const QString &h : hostnames) {
        if (NetworkingValidation::isIp(h) || NetworkingValidation::isDomainWithWildcard(h)) {
            filtered.append(h);
        } else {
            qCWarning(LOG_BASIC) << "ConnectedDnsInfo: dropping invalid split-DNS hostname";
        }
    }
    hostnames = filtered;

    // Clear fields that are not relevant to the selected type.
    if (type == CONNECTED_DNS_TYPE_AUTO || type == CONNECTED_DNS_TYPE_LOCAL) {
        upStream1.clear();
        isSplitDns = false;
        controldApiKey.clear();
        controldDevices.clear();
    } else if (type == CONNECTED_DNS_TYPE_CUSTOM) {
        controldApiKey.clear();
        controldDevices.clear();
    }
    if (!isSplitDns) {
        upStream2.clear();
        hostnames.clear();
    }
}

bool ConnectedDnsInfo::operator==(const ConnectedDnsInfo &other) const
{
    return other.type == type &&
           other.upStream1 == upStream1 &&
           other.isSplitDns == isSplitDns &&
           other.upStream2 == upStream2 &&
           other.hostnames == hostnames &&
           other.controldApiKey == controldApiKey &&
           other.controldDevices == controldDevices;
}

bool ConnectedDnsInfo::operator!=(const ConnectedDnsInfo &other) const
{
    return !(*this == other);
}

QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o)
{
    stream << o.versionForSerialization_;
    stream << o.type << o.upStream1 << o.isSplitDns << o.upStream2 << o.hostnames << o.controldApiKey << o.controldDevices;
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
    else if (version == 2)
        stream >> o.type >> o.upStream1 >> o.isSplitDns >> o.upStream2 >> o.hostnames;
    else
        stream >> o.type >> o.upStream1 >> o.isSplitDns >> o.upStream2 >> o.hostnames >> o.controldApiKey >> o.controldDevices;

    return stream;
}

} // types namespace
