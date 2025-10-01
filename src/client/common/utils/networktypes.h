#pragma once

#include <QDebug>
#include <QString>
#include "types/enums.h"

// TODO: convert unsigned long to int where appropriate
// TODO: simplify these types

struct IpForwardRow
{
    unsigned long index;
    unsigned long metric;
    bool valid;

    IpForwardRow() : index(0), metric(0), valid(false) {}
    IpForwardRow(unsigned long ind, unsigned long metric) : index(ind), metric(metric), valid(true) { }

    void print()
    {
        qDebug() << "IpForwardRow: " << index
                 << ", Metric: " << metric;
    }
};

struct IfTableRow
{
    int index;
    QString guidName;
    QString interfaceName;
    QString physicalAddress;
    NETWORK_INTERFACE_TYPE type;
    int dwType;
    int mtu;
    bool connected;
    int state;
    bool valid;

    IfTableRow() : index(-1), guidName(""), interfaceName(""), physicalAddress(""), type(NETWORK_INTERFACE_NONE), dwType(0), mtu(0), connected(false), state(0), valid(false) {}
    IfTableRow(int ind, QString n, QString intName, QString physAddress,  NETWORK_INTERFACE_TYPE typ, int dwType, int mtu, bool connected, int state) :
        index(ind), guidName(truncateGUID(n)), interfaceName(intName), physicalAddress(physAddress), type(typ), dwType(dwType), mtu(mtu), connected(connected), state(state), valid(true) {}

    void print()
    {
        qDebug() << "IfTableRow: " << index
                 << ", GUID: " << guidName
                 << ", Interface: " << interfaceName
                 << ", Physical: " << physicalAddress
                 << ", Nic Type: " << type
                 << ", DW Type: " << dwType
                 << ", MTU: " << mtu
                 << ", Connected: " << connected
                 << ", State: " << state;
    }

    QString truncateGUID(QString headerPlusGUID)
    {
        QString result = "";
        int start = headerPlusGUID.indexOf('{');
        result = headerPlusGUID.mid(start);

        return result;
    }
};

struct IfTable2Row
{
    unsigned long index;
    QString interfaceGuid;
    QString description;
    QString alias;
    int op_status;
    bool connectorPresent;
    bool endPointInterface;
    bool valid;
    unsigned long interfaceType;
    bool mediaConnected;

    IfTable2Row(): index(0), interfaceGuid(""), description(""), alias(""), op_status(0), connectorPresent(false), endPointInterface(false), valid(false), interfaceType(0), mediaConnected(false) {}
    IfTable2Row(unsigned long index, QString guid, QString description, QString alias, int op_status, bool connectorPresent, bool endPointInterface, unsigned long interfaceType, bool mediaConnected) :
        index(index), interfaceGuid(guid), description(description), alias(alias), op_status(op_status), connectorPresent(connectorPresent),
        endPointInterface(endPointInterface), valid(true), interfaceType(interfaceType), mediaConnected(mediaConnected) {}

    bool isWindscribeAdapter() const
    {
        // Warning: we control the alias of the wireguard-nt adapter, but not the description.
        return description.contains("windscribe", Qt::CaseInsensitive) || alias.contains("windscribe", Qt::CaseInsensitive);
    }

    void print()
    {
        qDebug() << "IfTable2Row: " << index
                 << ", GUID: " << interfaceGuid
                 << ", description: " << description
                 << ", alias: " << alias
                 << ", op status: " << op_status
                 << ", interfaceType: " << interfaceType
                 << ", mediaConnected: " << mediaConnected;
    }
};

struct AdapterAddress
{
    unsigned long index;
    QString networkGuid;
    QString friendlyName;
    // QString interfaceName;
    bool valid;

    AdapterAddress() : index(0), networkGuid(""), friendlyName(""), valid(false) {}
    AdapterAddress(unsigned long index, QString networkGuid, QString friendlyName) : index(index), networkGuid(networkGuid), friendlyName(friendlyName), valid(true) {}

    void print()
    {
        qDebug() << "Adapter Address: " << index
                 << ", network GUID: " << networkGuid
                 << ", FriendlyName: " << friendlyName;
    }

};

struct IpAdapter
{
    unsigned long index;
    QString guid;
    QString interfaceName; // device description
    QString physicalAddress;
    bool valid;

    IpAdapter() : index(0), guid(""), interfaceName(""), valid(false) {}
    IpAdapter(unsigned long ind, QString nam, QString desc, QString physicalAddress) : index(ind), guid(nam), interfaceName(desc), physicalAddress(physicalAddress), valid(true) {}

    void print()
    {
        qDebug() << "IpAdapter: " << index
                 << ", guid: " << guid
                 << ", interface: " << interfaceName
                 << ", physical: " << physicalAddress;
    }
};
