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
    NetworkInterface() :
        interfaceIndex(-1),
        interfaceType(NETWORK_INTERFACE_NONE),
        trustType(NETWORK_TRUST_SECURED),
        active(false),
        requested(false),
        metric(100),
        mtu(1470),
        state(0),
        dwType(0),
        connectorPresent(false),
        endPointInterface(false)
    {}

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

        if (json.contains(kJsonRequestedProp) && json[kJsonRequestedProp].isBool()) {
            requested = json[kJsonRequestedProp].toBool();
        }

        if (json.contains(kJsonMetricProp) && json[kJsonMetricProp].isDouble()) {
            metric = static_cast<int>(json[kJsonMetricProp].toDouble(100));
        }

        if (json.contains(kJsonPhysicalAddressProp) && json[kJsonPhysicalAddressProp].isString()) {
            physicalAddress = Utils::fromBase64(json[kJsonPhysicalAddressProp].toString());
        }

        if (json.contains(kJsonMtuProp) && json[kJsonMtuProp].isDouble()) {
            mtu = static_cast<int>(json[kJsonMtuProp].toDouble(1470));
        }

        if (json.contains(kJsonStateProp) && json[kJsonStateProp].isDouble()) {
            state = static_cast<int>(json[kJsonStateProp].toDouble(0));
        }

        if (json.contains(kJsonDwTypeProp) && json[kJsonDwTypeProp].isDouble()) {
            dwType = static_cast<int>(json[kJsonDwTypeProp].toDouble(0));
        }

        if (json.contains(kJsonDeviceNameProp) && json[kJsonDeviceNameProp].isString()) {
            deviceName = Utils::fromBase64(json[kJsonDeviceNameProp].toString());
        }

        if (json.contains(kJsonConnectorPresentProp) && json[kJsonConnectorPresentProp].isBool()) {
            connectorPresent = json[kJsonConnectorPresentProp].toBool();
        }

        if (json.contains(kJsonEndPointInterfaceProp) && json[kJsonEndPointInterfaceProp].isBool()) {
            endPointInterface = json[kJsonEndPointInterfaceProp].toBool();
        }
    }

    int interfaceIndex;
    QString interfaceName;
    QString interfaceGuid;
    QString networkOrSsid;
    NETWORK_INTERFACE_TYPE interfaceType;
    NETWORK_TRUST_TYPE trustType;
    bool active;
    QString friendlyName;
    bool requested;
    int metric;
    QString physicalAddress;
    int mtu;
    int state;
    int dwType;
    QString deviceName;
    bool connectorPresent;
    bool endPointInterface;

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
               other.requested == requested &&
               other.metric == metric &&
               other.physicalAddress == physicalAddress &&
               other.mtu == mtu &&
               other.state == state &&
               other.dwType == dwType &&
               other.deviceName == deviceName &&
               other.connectorPresent == connectorPresent &&
               other.endPointInterface == endPointInterface;
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

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[kJsonInterfaceIndexProp] = interfaceIndex;
        json[kJsonInterfaceNameProp] = Utils::toBase64(interfaceName);
        json[kJsonInterfaceGuidProp] = Utils::toBase64(interfaceGuid);
        json[kJsonNetworkOrSsidProp] = Utils::toBase64(networkOrSsid);
        json[kJsonInterfaceTypeProp] = static_cast<int>(interfaceType);
        json[kJsonTrustTypeProp] = static_cast<int>(trustType);
        json[kJsonActiveProp] = active;
        json[kJsonFriendlyNameProp] = Utils::toBase64(friendlyName);
        json[kJsonRequestedProp] = requested;
        json[kJsonMetricProp] = metric;
        json[kJsonPhysicalAddressProp] = Utils::toBase64(physicalAddress);
        json[kJsonMtuProp] = mtu;
        json[kJsonStateProp] = state;
        json[kJsonDwTypeProp] = dwType;
        json[kJsonDeviceNameProp] = Utils::toBase64(deviceName);
        json[kJsonConnectorPresentProp] = connectorPresent;
        json[kJsonEndPointInterfaceProp] = endPointInterface;
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
                  o.friendlyName << o.requested << o.metric << o.physicalAddress << o.mtu << o.state << o.dwType << o.deviceName <<
                  o.connectorPresent << o.endPointInterface;
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
                  o.friendlyName >> o.requested >> o.metric >> o.physicalAddress >> o.mtu >> o.state >> o.dwType >> o.deviceName >>
                  o.connectorPresent >> o.endPointInterface;
        return stream;
    }


    friend QDebug operator<<(QDebug dbg, const NetworkInterface &ni)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{friendlyName:" << ni.friendlyName << "; ";
        dbg << "networkOrSsid:" << ni.networkOrSsid << "}";
        return dbg;
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
    static const inline QString kJsonRequestedProp = "requested";
    static const inline QString kJsonMetricProp = "metric";
    static const inline QString kJsonPhysicalAddressProp = "physicalAddress";
    static const inline QString kJsonMtuProp = "mtu";
    static const inline QString kJsonStateProp = "state";
    static const inline QString kJsonDwTypeProp = "dwType";
    static const inline QString kJsonDeviceNameProp = "deviceName";
    static const inline QString kJsonConnectorPresentProp = "connectorPresent";
    static const inline QString kJsonEndPointInterfaceProp = "endPointInterface";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
