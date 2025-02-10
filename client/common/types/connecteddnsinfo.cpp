#include "connecteddnsinfo.h"
#include <QObject>
#include "utils/ws_assert.h"
#include "utils/ipvalidation.h"

namespace types {

ConnectedDnsInfo::ConnectedDnsInfo(const QJsonObject &json)
{
    if (json.contains(kJsonTypeProp) && json[kJsonTypeProp].isDouble()) {
        type = CONNECTED_DNS_TYPE_fromInt(json[kJsonTypeProp].toInt());
#ifndef Q_OS_WIN
        // non-Windows platforms do not have 'Forced' DNS mode
        if (type == CONNECTED_DNS_TYPE_FORCED) {
            type = CONNECTED_DNS_TYPE_AUTO;
        }
#endif
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
}

QList<CONNECTED_DNS_TYPE> ConnectedDnsInfo::allAvailableTypes()
{
    QList<CONNECTED_DNS_TYPE> t;
    t << CONNECTED_DNS_TYPE_AUTO << CONNECTED_DNS_TYPE_FORCED << CONNECTED_DNS_TYPE_CUSTOM;
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

    return json;
}

void ConnectedDnsInfo::fromIni(const QSettings &settings)
{
    type = CONNECTED_DNS_TYPE_fromString(settings.value(kIniTypeProp, "Auto").toString());
#ifndef Q_OS_WIN
        // non-Windows platforms do not have 'Forced' DNS mode
        if (type == CONNECTED_DNS_TYPE_FORCED) {
            type = CONNECTED_DNS_TYPE_AUTO;
        }
#endif

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
    settings.setValue(kIniTypeProp, CONNECTED_DNS_TYPE_toString(type));
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
