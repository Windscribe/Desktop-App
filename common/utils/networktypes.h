#ifndef NETWORKTYPES_H
#define NETWORKTYPES_H

#include <QDebug>
#include <QString>
#include "utils/protobuf_includes.h"

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
    ProtoTypes::NetworkInterfaceType type;
    int dwType;
    int mtu;
    bool connected;
    int state;
    bool valid;

    IfTableRow() : index(-1), guidName(""), interfaceName(""), physicalAddress(""), type(ProtoTypes::NETWORK_INTERFACE_NONE), dwType(0), mtu(0), connected(false), state(0), valid(false) {}
    IfTableRow(int ind, QString n, QString intName, QString physAddress,  ProtoTypes::NetworkInterfaceType typ, int dwType, int mtu, bool connected, int state) :
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
    QString alias;
    int op_status;
    bool connectorPresent;
    bool endPointInterface;

    IfTable2Row(): index(0), interfaceGuid(""), alias(""), op_status(0), connectorPresent(false), endPointInterface(false) {}
    IfTable2Row(unsigned long index, QString guid, QString alias, int op_status, bool connectorPresent, bool endPointInterface) :
        index(index), interfaceGuid(guid), alias(alias), op_status(op_status), connectorPresent(connectorPresent), endPointInterface(endPointInterface) {}

    void print()
    {
        qDebug() << "IfTable2Row: " << index
                 << ", GUID: " << interfaceGuid
                 << ", alias: " << alias
                 << ", op status: " << op_status;
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


#endif // NETWORKTYPES_H
