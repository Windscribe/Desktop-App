#ifndef TYPES_MACADDRSPOOFING_H
#define TYPES_MACADDRSPOOFING_H

#include <QJsonArray>
#include <QString>
#include "networkinterface.h"

namespace types {

struct MacAddrSpoofing
{
    MacAddrSpoofing() :
        isEnabled(false),
        isAutoRotate(false)
    {}

    bool isEnabled;
    QString macAddress;
    bool isAutoRotate;
    NetworkInterface selectedNetworkInterface;
    QVector<NetworkInterface> networkInterfaces;

    bool operator==(const MacAddrSpoofing &other) const
    {
        return other.isEnabled == isEnabled &&
               other.macAddress == macAddress &&
               other.isAutoRotate == isAutoRotate &&
               other.selectedNetworkInterface == selectedNetworkInterface &&
               other.networkInterfaces == networkInterfaces;
    }

    bool operator!=(const MacAddrSpoofing &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["isEnabled"] = isEnabled;
        json["macAddress"] = macAddress;
        json["isAutoRotate"] = isAutoRotate;
        json["selectedNetworkInterface"] = selectedNetworkInterface.toJsonObject();

        QJsonArray arr;
        for (const auto &it : networkInterfaces) {
            arr << it.toJsonObject();
        }
        json["networkInterfaces"] = arr;
        return json;
    }

    bool fromJsonObject(const QJsonObject &json)
    {
        if (json.contains("isEnabled")) isEnabled = json["isEnabled"].toBool(false);
        if (json.contains("macAddress")) macAddress = json["macAddress"].toString();
        if (json.contains("isAutoRotate")) isAutoRotate = json["isAutoRotate"].toBool(false);
        if (json.contains("selectedNetworkInterface")) selectedNetworkInterface.fromJsonObject(json["selectedNetworkInterface"].toObject());
        if (json.contains("networkInterfaces"))
        {
            QJsonArray arr = json["selectedNetworkInterface"].toArray();
            for (const auto it : arr)
            {
                NetworkInterface ni(it.toObject());
                networkInterfaces << ni;
            }
        }

        return true;
    }
};


} // types namespace

#endif // TYPES_MACADDRSPOOFING_H
