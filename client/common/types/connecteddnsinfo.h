#pragma once

#include <QString>
#include <QList>
#include <QDataStream>
#include <QJsonArray>
#include <QJsonObject>
#include "enums.h"

namespace types {

struct ConnectedDnsInfo
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kTypeProp = "type";
        const QString kUpStream1Prop = "upStream1";
        const QString kIsSplitDnsProp = "isSplitDns";
        const QString kUpStream2Prop = "upStream2";
        const QString kHostnamesProp = "hostnames";
    };

    CONNECTED_DNS_TYPE type = CONNECTED_DNS_TYPE_AUTO;
    QString upStream1;
    bool isSplitDns = false;
    QString upStream2;
    QStringList hostnames;
    JsonInfo jsonInfo;

    ConnectedDnsInfo() = default;

    ConnectedDnsInfo(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kTypeProp) && json[jsonInfo.kTypeProp].isDouble())
            type = static_cast<CONNECTED_DNS_TYPE>(json[jsonInfo.kTypeProp].toInt());

        if (json.contains(jsonInfo.kUpStream1Prop) && json[jsonInfo.kUpStream1Prop].isString())
            upStream1 = json[jsonInfo.kUpStream1Prop].toString();

        if (json.contains(jsonInfo.kIsSplitDnsProp) && json[jsonInfo.kIsSplitDnsProp].isBool())
            isSplitDns = json[jsonInfo.kIsSplitDnsProp].toBool();

        if (json.contains(jsonInfo.kUpStream2Prop) && json[jsonInfo.kUpStream2Prop].isString())
            upStream2 = json[jsonInfo.kUpStream2Prop].toString();

        if (json.contains(jsonInfo.kHostnamesProp) && json[jsonInfo.kHostnamesProp].isArray())
        {
            QJsonArray hostnamesArray = json[jsonInfo.kHostnamesProp].toArray();
            hostnames.clear();
            for (const QJsonValue &hostnameValue : hostnamesArray)
            {
                if (hostnameValue.isString())
                    hostnames.append(hostnameValue.toString());
            }
        }
    }


    static QList<CONNECTED_DNS_TYPE> allAvailableTypes();
    static QString typeToString(const CONNECTED_DNS_TYPE &type);

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kTypeProp] = static_cast<int>(type);
        json[jsonInfo.kUpStream1Prop] = upStream1;
        json[jsonInfo.kIsSplitDnsProp] = isSplitDns;
        json[jsonInfo.kUpStream2Prop] = upStream2;

        QJsonArray hostnamesArray;
        for (const QString& hostname : hostnames) {
            hostnamesArray.append(hostname);
        }
        json[jsonInfo.kHostnamesProp] = hostnamesArray;

        return json;
    }

    bool isCustomIPv4Address() const;

    friend QDataStream& operator <<(QDataStream &stream, const ConnectedDnsInfo &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectedDnsInfo &o);

    friend QDebug operator<<(QDebug dbg, const ConnectedDnsInfo &ds);

    bool operator==(const ConnectedDnsInfo &other) const;
    bool operator!=(const ConnectedDnsInfo &other) const;

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

} // types namespace
