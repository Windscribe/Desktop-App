#ifndef TYPES_MACADDRSPOOFING_H
#define TYPES_MACADDRSPOOFING_H

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

};


} // types namespace

#endif // TYPES_MACADDRSPOOFING_H
