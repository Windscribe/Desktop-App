#ifndef TYPES_NETWORKINTERFACE_H
#define TYPES_NETWORKINTERFACE_H

#include <QDataStream>
#include <QJsonObject>
#include <QString>
#include "enums.h"

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

    explicit NetworkInterface(const QJsonObject &json)
    {
        fromJsonObject(json);
    }

    int interfaceIndex;
    QString interfaceName;
    QString interfaceGuid;
    QString networkOrSSid;
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

    bool operator==(const NetworkInterface &other) const
    {
        return other.interfaceIndex == interfaceIndex &&
               other.interfaceName == interfaceName &&
               other.interfaceGuid == interfaceGuid &&
               other.networkOrSSid == networkOrSSid &&
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

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["interfaceIndex"] = interfaceIndex;
        json["interfaceName"] = interfaceName;
        json["interfaceGuid"] = interfaceGuid;
        json["networkOrSSid"] = networkOrSSid;
        json["interfaceType"] = (int)interfaceType;
        json["trustType"] = (int)trustType;
        json["active"] = active;
        json["friendlyName"] = friendlyName;
        json["requested"] = requested;
        json["metric"] = metric;
        json["physicalAddress"] = physicalAddress;
        json["mtu"] = mtu;
        json["state"] = state;
        json["dwType"] = dwType;
        json["deviceName"] = deviceName;
        json["connectorPresent"] = connectorPresent;
        json["endPointInterface"] = endPointInterface;
        return json;
    }

    bool fromJsonObject(const QJsonObject &json)
    {
        if (json.contains("interfaceIndex")) interfaceIndex = json["interfaceIndex"].toInt(-1);
        if (json.contains("interfaceName")) interfaceName = json["interfaceName"].toString();
        if (json.contains("interfaceGuid")) interfaceGuid = json["interfaceGuid"].toString();
        if (json.contains("networkOrSSid")) networkOrSSid = json["networkOrSSid"].toString();
        if (json.contains("interfaceType")) interfaceType = (NETWORK_INTERACE_TYPE)json["interfaceType"].toInt(NETWORK_INTERFACE_NONE);
        if (json.contains("trustType")) trustType = (NETWORK_TRUST_TYPE)json["trustType"].toInt(NETWORK_TRUST_SECURED);
        if (json.contains("active")) active = json["active"].toBool(false);
        if (json.contains("friendlyName")) friendlyName = json["friendlyName"].toString();
        if (json.contains("requested")) requested = json["requested"].toBool(false);
        if (json.contains("metric")) metric = json["metric"].toInt(100);
        if (json.contains("physicalAddress")) physicalAddress = json["physicalAddress"].toString();
        if (json.contains("mtu")) mtu = json["mtu"].toInt(1470);
        if (json.contains("state")) state = json["state"].toInt(0);
        if (json.contains("dwType")) dwType = json["dwType"].toInt(0);
        if (json.contains("deviceName")) deviceName = json["deviceName"].toString();
        if (json.contains("connectorPresent")) connectorPresent = json["connectorPresent"].toBool(false);
        if (json.contains("endPointInterface")) endPointInterface = json["endPointInterface"].toBool(false);
        return true;
    }

    friend QDebug operator<<(QDebug dbg, const NetworkInterface &ni)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{friendlyName:" << ni.friendlyName << "; ";
        dbg << "networkOrSSid:" << ni.networkOrSSid << "}";
        return dbg;
    }

};


} // types namespace

#endif // TYPES_NETWORKINTERFACE_H
