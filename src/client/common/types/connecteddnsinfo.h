#pragma once

#include <QDataStream>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QSettings>
#include <QString>
#include "enums.h"

namespace types {

struct ConnectedDnsInfo
{
    CONNECTED_DNS_TYPE type = CONNECTED_DNS_TYPE_AUTO;
    QString upStream1;
    bool isSplitDns = false;
    QString upStream2;
    QStringList hostnames;
    QString controldApiKey;
    QList<QPair<QString, QString>> controldDevices;  // List of (name, doh_resolver) pairs

    ConnectedDnsInfo() = default;
    ConnectedDnsInfo(const QJsonObject &json);

    static QList<CONNECTED_DNS_TYPE> allAvailableTypes();

    QJsonObject toJson() const;
    void fromIni(const QSettings &settings);
    void toIni(QSettings &settings) const;

    bool isCustomIPv4Address() const;

    friend QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectedDnsInfo &o);

    bool operator==(const ConnectedDnsInfo &other) const;
    bool operator!=(const ConnectedDnsInfo &other) const;

    static const inline QString kIniTypeProp = "ConnectedDNSMode";
    static const inline QString kIniUpStream1Prop = "ConnectedDNSUpstream1";
    static const inline QString kIniIsSplitDnsProp = "SplitDNS";
    static const inline QString kIniUpStream2Prop = "ConnectedDNSUpstream2";
    static const inline QString kIniHostnamesProp = "SplitDNSHostnames";

    static const inline QString kJsonTypeProp = "type";
    static const inline QString kJsonUpStream1Prop = "upStream1";
    static const inline QString kJsonIsSplitDnsProp = "isSplitDns";
    static const inline QString kJsonUpStream2Prop = "upStream2";
    static const inline QString kJsonHostnamesProp = "hostnames";
    static const inline QString kJsonControldApiKeyProp = "controldApiKey";

    static constexpr quint32 versionForSerialization_ = 3;  // should increment the version if the data format is changed
};

} // types namespace
