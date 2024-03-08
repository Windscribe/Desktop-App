#pragma once

#include <QDataStream>
#include <QJsonObject>
#include <QString>

#include "enums.h"

namespace types {

struct NetworkInterface
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kInterfaceIndexProp = "interfaceIndex";
        const QString kInterfaceNameProp = "interfaceName";
        const QString kInterfaceGuidProp = "interfaceGuid";
        const QString kNetworkOrSsidProp = "networkOrSsid";
        const QString kInterfaceTypeProp = "interfaceType";
        const QString kTrustTypeProp = "trustType";
        const QString kActiveProp = "active";
        const QString kFriendlyNameProp = "friendlyName";
        const QString kRequestedProp = "requested";
        const QString kMetricProp = "metric";
        const QString kPhysicalAddressProp = "physicalAddress";
        const QString kMtuProp = "mtu";
        const QString kStateProp = "state";
        const QString kDwTypeProp = "dwType";
        const QString kDeviceNameProp = "deviceName";
        const QString kConnectorPresentProp = "connectorPresent";
        const QString kEndPointInterfaceProp = "endPointInterface";
    };

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
        if (json.contains(jsonInfo.kInterfaceIndexProp) && json[jsonInfo.kInterfaceIndexProp].isDouble())
            interfaceIndex = static_cast<int>(json[jsonInfo.kInterfaceIndexProp].toDouble(-1));

        if (json.contains(jsonInfo.kInterfaceNameProp) && json[jsonInfo.kInterfaceNameProp].isString())
            interfaceName = fromBase64(json[jsonInfo.kInterfaceNameProp].toString());

        if (json.contains(jsonInfo.kInterfaceGuidProp) && json[jsonInfo.kInterfaceGuidProp].isString())
            interfaceGuid = fromBase64(json[jsonInfo.kInterfaceGuidProp].toString());

        if (json.contains(jsonInfo.kNetworkOrSsidProp) && json[jsonInfo.kNetworkOrSsidProp].isString())
            networkOrSsid = fromBase64(json[jsonInfo.kNetworkOrSsidProp].toString());

        if (json.contains(jsonInfo.kInterfaceTypeProp) && json[jsonInfo.kInterfaceTypeProp].isDouble())
            interfaceType = static_cast<NETWORK_INTERACE_TYPE>(json[jsonInfo.kInterfaceTypeProp].toInt(NETWORK_INTERFACE_NONE));

        if (json.contains(jsonInfo.kTrustTypeProp) && json[jsonInfo.kTrustTypeProp].isDouble())
            trustType = static_cast<NETWORK_TRUST_TYPE>(json[jsonInfo.kTrustTypeProp].toInt(NETWORK_TRUST_SECURED));

        if (json.contains(jsonInfo.kActiveProp) && json[jsonInfo.kActiveProp].isBool())
            active = json[jsonInfo.kActiveProp].toBool();

        if (json.contains(jsonInfo.kFriendlyNameProp) && json[jsonInfo.kFriendlyNameProp].isString())
            friendlyName = fromBase64(json[jsonInfo.kFriendlyNameProp].toString());

        if (json.contains(jsonInfo.kRequestedProp) && json[jsonInfo.kRequestedProp].isBool())
            requested = json[jsonInfo.kRequestedProp].toBool();

        if (json.contains(jsonInfo.kMetricProp) && json[jsonInfo.kMetricProp].isDouble())
            metric = static_cast<int>(json[jsonInfo.kMetricProp].toDouble(100));

        if (json.contains(jsonInfo.kPhysicalAddressProp) && json[jsonInfo.kPhysicalAddressProp].isString())
            physicalAddress = fromBase64(json[jsonInfo.kPhysicalAddressProp].toString());

        if (json.contains(jsonInfo.kMtuProp) && json[jsonInfo.kMtuProp].isDouble())
            mtu = static_cast<int>(json[jsonInfo.kMtuProp].toDouble(1470));

        if (json.contains(jsonInfo.kStateProp) && json[jsonInfo.kStateProp].isDouble())
            state = static_cast<int>(json[jsonInfo.kStateProp].toDouble(0));

        if (json.contains(jsonInfo.kDwTypeProp) && json[jsonInfo.kDwTypeProp].isDouble())
            dwType = static_cast<int>(json[jsonInfo.kDwTypeProp].toDouble(0));

        if (json.contains(jsonInfo.kDeviceNameProp) && json[jsonInfo.kDeviceNameProp].isString())
            deviceName = fromBase64(json[jsonInfo.kDeviceNameProp].toString());

        if (json.contains(jsonInfo.kConnectorPresentProp) && json[jsonInfo.kConnectorPresentProp].isBool())
            connectorPresent = json[jsonInfo.kConnectorPresentProp].toBool();

        if (json.contains(jsonInfo.kEndPointInterfaceProp) && json[jsonInfo.kEndPointInterfaceProp].isBool())
            endPointInterface = json[jsonInfo.kEndPointInterfaceProp].toBool();
    }

    int interfaceIndex;
    QString interfaceName;
    QString interfaceGuid;
    QString networkOrSsid;
    NETWORK_INTERACE_TYPE interfaceType;
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
    JsonInfo jsonInfo;

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

    QString toBase64(const QString& str) const
    {
        return QString(str.toUtf8().toBase64());
    }

    QString fromBase64(const QString& str) const
    {
        return QString::fromUtf8(QByteArray::fromBase64(str.toUtf8()));
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kInterfaceIndexProp] = interfaceIndex;
        json[jsonInfo.kInterfaceNameProp] = toBase64(interfaceName);
        json[jsonInfo.kInterfaceGuidProp] = toBase64(interfaceGuid);
        json[jsonInfo.kNetworkOrSsidProp] = toBase64(networkOrSsid);
        json[jsonInfo.kInterfaceTypeProp] = static_cast<int>(interfaceType);
        json[jsonInfo.kTrustTypeProp] = static_cast<int>(trustType);
        json[jsonInfo.kActiveProp] = active;
        json[jsonInfo.kFriendlyNameProp] = toBase64(friendlyName);
        json[jsonInfo.kRequestedProp] = requested;
        json[jsonInfo.kMetricProp] = metric;
        json[jsonInfo.kPhysicalAddressProp] = toBase64(physicalAddress);
        json[jsonInfo.kMtuProp] = mtu;
        json[jsonInfo.kStateProp] = state;
        json[jsonInfo.kDwTypeProp] = dwType;
        json[jsonInfo.kDeviceNameProp] = toBase64(deviceName);
        json[jsonInfo.kConnectorPresentProp] = connectorPresent;
        json[jsonInfo.kEndPointInterfaceProp] = endPointInterface;
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
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
