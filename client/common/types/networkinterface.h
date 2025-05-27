#pragma once

#include <QDataStream>
#include <QJsonObject>
#include <QString>

#include "enums.h"
#include "utils/utils.h"

namespace types {

struct NetworkInterface
{
    // defaults
    NetworkInterface() {}

    NetworkInterface(const QJsonObject &json)
    {
        if (json.contains(kJsonInterfaceIndexProp) && json[kJsonInterfaceIndexProp].isDouble()) {
            interfaceIndex = static_cast<int>(json[kJsonInterfaceIndexProp].toDouble(-1));
        }

        if (json.contains(kJsonInterfaceNameProp) && json[kJsonInterfaceNameProp].isString()) {
            interfaceName = Utils::fromBase64(json[kJsonInterfaceNameProp].toString());
        }

        if (json.contains(kJsonInterfaceGuidProp) && json[kJsonInterfaceGuidProp].isString()) {
            interfaceGuid = Utils::fromBase64(json[kJsonInterfaceGuidProp].toString());
        }

        if (json.contains(kJsonNetworkOrSsidProp) && json[kJsonNetworkOrSsidProp].isString()) {
            networkOrSsid = Utils::fromBase64(json[kJsonNetworkOrSsidProp].toString());
        }

        if (json.contains(kJsonInterfaceTypeProp) && json[kJsonInterfaceTypeProp].isDouble()) {
            interfaceType = NETWORK_INTERFACE_TYPE_fromInt(json[kJsonInterfaceTypeProp].toInt(NETWORK_INTERFACE_NONE));
        }

        if (json.contains(kJsonTrustTypeProp) && json[kJsonTrustTypeProp].isDouble()) {
            trustType = NETWORK_TRUST_TYPE_fromInt(json[kJsonTrustTypeProp].toInt(NETWORK_TRUST_SECURED));
        }

        if (json.contains(kJsonActiveProp) && json[kJsonActiveProp].isBool()) {
            active = json[kJsonActiveProp].toBool();
        }

        if (json.contains(kJsonFriendlyNameProp) && json[kJsonFriendlyNameProp].isString()) {
            friendlyName = Utils::fromBase64(json[kJsonFriendlyNameProp].toString());
        }

        if (json.contains(kJsonMetricProp) && json[kJsonMetricProp].isDouble()) {
            metric = static_cast<int>(json[kJsonMetricProp].toDouble(100));
        }

        if (json.contains(kJsonPhysicalAddressProp) && json[kJsonPhysicalAddressProp].isString()) {
            physicalAddress = Utils::fromBase64(json[kJsonPhysicalAddressProp].toString());
        }

        if (json.contains(kJsonMtuProp) && json[kJsonMtuProp].isDouble()) {
            int mtuValue = json[kJsonMtuProp].toInt();
            // MTU is unset, or must be a minimum of 68 per RFC 791 and can't exceed maximum IP packet size of 65535
            if (mtuValue < 0 || (mtuValue >= 68 && mtuValue < 65536)) {
                mtu = mtuValue;
            }
        }
    }

    int interfaceIndex = -1;
    QString interfaceName;
    QString interfaceGuid;
    QString networkOrSsid;
    NETWORK_INTERFACE_TYPE interfaceType = NETWORK_INTERFACE_NONE;
    NETWORK_TRUST_TYPE trustType = NETWORK_TRUST_SECURED;
    bool active = false;
    QString friendlyName;
    int metric = 100;
    QString physicalAddress;
    int mtu = 1470;

    bool operator==(const NetworkInterface &other) const
    {
        return other.interfaceIndex == interfaceIndex &&
               other.interfaceName == interfaceName &&
               other.interfaceGuid == interfaceGuid &&
               other.networkOrSsid == networkOrSsid &&
               other.interfaceType == interfaceType &&
               other.trustType == trustType &&
               other.active == active &&
               other.friendlyName == friendlyName &&
               other.metric == metric &&
               other.physicalAddress == physicalAddress &&
               other.mtu == mtu;
    }

    bool operator!=(const NetworkInterface &other) const
    {
        return !(*this == other);
    }

    bool isValid() const
    {
        return (interfaceIndex != -1 && !physicalAddress.isEmpty()) || isNoNetworkInterface();
    }

    static NetworkInterface noNetworkInterface()
    {
        NetworkInterface iff;
        iff.interfaceIndex = -1;
        iff.interfaceName = "No Interface";
        iff.interfaceType = NETWORK_INTERFACE_NONE;
        return iff;
    }

    bool isNoNetworkInterface() const
    {
        return (interfaceIndex == -1 && interfaceType == NETWORK_INTERFACE_NONE && interfaceName == "No Interface");
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        if (isForDebugLog) {
            json[kJsonNetworkOrSsidProp] = networkOrSsid;
            json[kJsonFriendlyNameProp] = friendlyName;
        } else {
            json[kJsonInterfaceIndexProp] = interfaceIndex;
            json[kJsonInterfaceNameProp] = Utils::toBase64(interfaceName);
            json[kJsonInterfaceGuidProp] = Utils::toBase64(interfaceGuid);
            json[kJsonNetworkOrSsidProp] = Utils::toBase64(networkOrSsid);
            json[kJsonInterfaceTypeProp] = static_cast<int>(interfaceType);
            json[kJsonTrustTypeProp] = static_cast<int>(trustType);
            json[kJsonActiveProp] = active;
            json[kJsonFriendlyNameProp] = Utils::toBase64(friendlyName);
            json[kJsonMetricProp] = metric;
            json[kJsonPhysicalAddressProp] = Utils::toBase64(physicalAddress);
            json[kJsonMtuProp] = mtu;
        }
        return json;
    }

    static bool isNoNetworkInterface(int interfaceIndex)
    {
        return (interfaceIndex == noNetworkInterface().interfaceIndex);
    }

    bool sameNetworkInterface(const types::NetworkInterface &interface2)
    {
        if (interfaceIndex != interface2.interfaceIndex)    return false;
        else if (interfaceName != interface2.interfaceName) return false;
        else if (interfaceGuid != interface2.interfaceGuid) return false;
        else if (networkOrSsid != interface2.networkOrSsid) return false;
        else if (friendlyName != interface2.friendlyName)   return false;
        return true;
    }

    friend QDataStream& operator <<(QDataStream &stream, const NetworkInterface &o)
    {
        stream << versionForSerialization_;
        stream << o.interfaceIndex << o.interfaceName << o.interfaceGuid << o.networkOrSsid << o.interfaceType << o.trustType << o.active <<
                  o.friendlyName << o.metric << o.physicalAddress << o.mtu;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, NetworkInterface &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.interfaceIndex >> o.interfaceName >> o.interfaceGuid >> o.networkOrSsid >> o.interfaceType >> o.trustType >> o.active >>
                  o.friendlyName;

        if (version < 2) {
            bool requested;
            stream >> requested; // Deprecated
        }

        stream >> o.metric >> o.physicalAddress >> o.mtu;

        if (version < 2) {
            int dwType;
            int state;
            QString deviceName;
            bool connectorPresent;
            bool endPointInterface;
            stream >> state >> dwType >> deviceName >> connectorPresent >> endPointInterface; // Deprecated
        }
        return stream;
    }

private:
    static const inline QString kJsonInterfaceIndexProp = "interfaceIndex";
    static const inline QString kJsonInterfaceNameProp = "interfaceName";
    static const inline QString kJsonInterfaceGuidProp = "interfaceGuid";
    static const inline QString kJsonNetworkOrSsidProp = "networkOrSsid";
    static const inline QString kJsonInterfaceTypeProp = "interfaceType";
    static const inline QString kJsonTrustTypeProp = "trustType";
    static const inline QString kJsonActiveProp = "active";
    static const inline QString kJsonFriendlyNameProp = "friendlyName";
    static const inline QString kJsonMetricProp = "metric";
    static const inline QString kJsonPhysicalAddressProp = "physicalAddress";
    static const inline QString kJsonMtuProp = "mtu";

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

} // types namespace
