#include "connecteddnsinfo.h"
#include <QObject>
#include "utils/ws_assert.h"
#include "utils/ipvalidation.h"
#include "utils/utils.h"

namespace types {

ConnectedDnsInfo::ConnectedDnsInfo(const QJsonObject &json)
{
    if (json.contains(kJsonTypeProp) && json[kJsonTypeProp].isDouble()) {
        type = CONNECTED_DNS_TYPE_fromInt(json[kJsonTypeProp].toInt());
        // CONNECTED_DNS_TYPE_FORCED has been merged into CONNECTED_DNS_TYPE_AUTO
        if (type == CONNECTED_DNS_TYPE_FORCED) {
            type = CONNECTED_DNS_TYPE_AUTO;
        }
    }

    if (json.contains(kJsonUpStream1Prop) && json[kJsonUpStream1Prop].isString()) {
        QString server = json[kJsonUpStream1Prop].toString();
        if (IpValidation::isCtrldCorrectAddress(server)) {
            upStream1 = server;
        }
    }

    if (json.contains(kJsonIsSplitDnsProp) && json[kJsonIsSplitDnsProp].isBool()) {
        isSplitDns = json[kJsonIsSplitDnsProp].toBool();
    }

    if (json.contains(kJsonUpStream2Prop) && json[kJsonUpStream2Prop].isString()) {
        QString server = json[kJsonUpStream2Prop].toString();
        if (IpValidation::isCtrldCorrectAddress(server)) {
            upStream2 = server;
        }
    }

    if (json.contains(kJsonHostnamesProp) && json[kJsonHostnamesProp].isArray()) {
        QJsonArray hostnamesArray = json[kJsonHostnamesProp].toArray();
        hostnames.clear();
        for (const QJsonValue &hostnameValue : hostnamesArray) {
            if (hostnameValue.isString()) {
                QString hostname = hostnameValue.toString();
                if (IpValidation::isIp(hostname) || IpValidation::isDomainWithWildcard(hostname)) {
                    hostnames.append(hostname);
                }
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
    return type == CONNECTED_DNS_TYPE_CUSTOM && IpValidation::isIp(upStream1) && isSplitDns == false;
}


QJsonObject ConnectedDnsInfo::toJson() const
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

    json[kJsonControldApiKeyProp] = Utils::toBase64(controldApiKey);

    return json;
}

void ConnectedDnsInfo::fromIni(const QSettings &settings)
{
    type = CONNECTED_DNS_TYPE_fromString(settings.value(kIniTypeProp, "Auto").toString());
    // CONNECTED_DNS_TYPE_FORCED has been merged into CONNECTED_DNS_TYPE_AUTO
    if (type == CONNECTED_DNS_TYPE_FORCED) {
        type = CONNECTED_DNS_TYPE_AUTO;
    }

    // If using CLI-only, this type is not supported, change it to custom instead.
    if (type == CONNECTED_DNS_TYPE_CONTROLD) {
        type = CONNECTED_DNS_TYPE_CUSTOM;
    }

    QString str = settings.value(kIniUpStream1Prop).toString();
    if (IpValidation::isCtrldCorrectAddress(str)) {
       upStream1 = str;
    }

    isSplitDns = settings.value(kIniIsSplitDnsProp, false).toBool();

    str = settings.value(kIniUpStream2Prop).toString();
    if (IpValidation::isCtrldCorrectAddress(str)) {
       upStream2 = str;
    }

    QStringList list = settings.value(kIniHostnamesProp).toStringList();
    QStringList domains;
    for (const QString &domain: list) {
        if (IpValidation::isIp(domain) || IpValidation::isDomainWithWildcard(domain)) {
            domains.append(domain);
        }
    }
    hostnames = domains;
}

void ConnectedDnsInfo::toIni(QSettings &settings) const
{
    // CONNECTED_DNS_TYPE_CONTROLD is not supported for INIs, change it to custom.
    CONNECTED_DNS_TYPE typeToWrite = type;
    if (typeToWrite == CONNECTED_DNS_TYPE_CONTROLD) {
        typeToWrite = CONNECTED_DNS_TYPE_CUSTOM;
    }

    settings.setValue(kIniTypeProp, CONNECTED_DNS_TYPE_toString(typeToWrite));
    settings.setValue(kIniUpStream1Prop, upStream1);
    settings.setValue(kIniIsSplitDnsProp, isSplitDns);
    settings.setValue(kIniUpStream2Prop, upStream2);
    if (hostnames.isEmpty()) {
        settings.setValue(kIniHostnamesProp, "");
    } else {
        settings.setValue(kIniHostnamesProp, hostnames);
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

    // CONNECTED_DNS_TYPE_FORCED has been merged into CONNECTED_DNS_TYPE_AUTO
    if (o.type == CONNECTED_DNS_TYPE_FORCED) {
        o.type = CONNECTED_DNS_TYPE_AUTO;
    }
    return stream;
}

} // types namespace
