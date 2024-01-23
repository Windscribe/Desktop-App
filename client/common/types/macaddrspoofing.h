#pragma once

#include <QJsonArray>
#include <QString>
#include "networkinterface.h"

namespace types {

struct MacAddrSpoofing
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kIsEnabledProp = "isEnabled";
        const QString kMacAddressProp = "macAddress";
        const QString kIsAutoRotateProp = "isAutoRotate";
        const QString kSelectedNetworkInterfaceProp = "selectedNetworkInterface";
        const QString kNetworkInterfacesProp = "networkInterfaces";
    };

    MacAddrSpoofing() :
        isEnabled(false),
        isAutoRotate(false)
    {}

    MacAddrSpoofing(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kIsEnabledProp) && json[jsonInfo.kIsEnabledProp].isBool())
            isEnabled = json[jsonInfo.kIsEnabledProp].toBool();

        if (json.contains(jsonInfo.kMacAddressProp) && json[jsonInfo.kMacAddressProp].isString())
            macAddress = json[jsonInfo.kMacAddressProp].toString();

        if (json.contains(jsonInfo.kIsAutoRotateProp) && json[jsonInfo.kIsAutoRotateProp].isBool())
            isAutoRotate = json[jsonInfo.kIsAutoRotateProp].toBool();

        if (json.contains(jsonInfo.kSelectedNetworkInterfaceProp) && json[jsonInfo.kSelectedNetworkInterfaceProp].isObject())
            selectedNetworkInterface = NetworkInterface(json[jsonInfo.kSelectedNetworkInterfaceProp].toObject());

        if (json.contains(jsonInfo.kNetworkInterfacesProp) && json[jsonInfo.kNetworkInterfacesProp].isArray())
        {
            QJsonArray networkInterfacesArray = json[jsonInfo.kNetworkInterfacesProp].toArray();
            networkInterfaces.clear();
            for (const QJsonValue &ifaceValue : networkInterfacesArray)
            {
                if (ifaceValue.isObject())
                {
                    NetworkInterface iface(ifaceValue.toObject());
                    networkInterfaces.append(iface);
                }
            }
        }
    }

    bool isEnabled;
    QString macAddress;
    bool isAutoRotate;
    NetworkInterface selectedNetworkInterface;
    QVector<NetworkInterface> networkInterfaces;
    JsonInfo jsonInfo;

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

    QJsonObject toJson() const
    {
        QJsonObject json;

        json[jsonInfo.kIsEnabledProp] = isEnabled;
        json[jsonInfo.kMacAddressProp] = macAddress;
        json[jsonInfo.kIsAutoRotateProp] = isAutoRotate;
        json[jsonInfo.kSelectedNetworkInterfaceProp] = selectedNetworkInterface.toJson();

        if (!networkInterfaces.isEmpty()) {
            QJsonArray networkInterfacesArray;
            for (const NetworkInterface& iface : networkInterfaces) {
                networkInterfacesArray.append(iface.toJson());
            }
            json[jsonInfo.kNetworkInterfacesProp] = networkInterfacesArray;
        }

        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const MacAddrSpoofing &o)
    {
        stream << versionForSerialization_;
        stream << o.isEnabled << o.macAddress << o.isAutoRotate << o.selectedNetworkInterface << o.networkInterfaces;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, MacAddrSpoofing &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isEnabled >> o.macAddress >> o.isAutoRotate >> o.selectedNetworkInterface >> o.networkInterfaces;
        return stream;
    }


    friend QDebug operator<<(QDebug dbg, const MacAddrSpoofing &m)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isEnabled:" << m.isEnabled << "; ";
        dbg << "macAddress:" << m.macAddress << "; ";
        dbg << "selectedNetworkInterface:" << m.selectedNetworkInterface << "; ";
        dbg << "networkInterfaces:{";
        for (const auto &i : m.networkInterfaces)
        {
            dbg << i;
        }

        dbg << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} // types namespace
