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
        Q_ASSERT(version == versionForSerialization_);
        if (version > versionForSerialization_)
        {
            return stream;
        }
        stream >> o.isEnabled >> o.macAddress >> o.isAutoRotate >> o.selectedNetworkInterface >> o.networkInterfaces;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;

};


} // types namespace

#endif // TYPES_MACADDRSPOOFING_H
