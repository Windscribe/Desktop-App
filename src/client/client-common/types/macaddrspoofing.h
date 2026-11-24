#pragma once

#include <QDataStream>
#include <QString>
#include <QVector>

#include "networkinterface.h"

namespace types {

struct MacAddrSpoofing
{
    bool isEnabled = false;
    QString macAddress;
    bool isAutoRotate = false;
    NetworkInterface selectedNetworkInterface;
    QVector<NetworkInterface> networkInterfaces;

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

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} // types namespace
