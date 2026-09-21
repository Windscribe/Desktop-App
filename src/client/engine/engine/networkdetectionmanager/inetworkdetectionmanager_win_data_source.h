#pragma once

#include <optional>

#include <QString>

#include "types/networkinterface.h"

class INetworkDetectionManager_winDataSource
{
public:
    virtual ~INetworkDetectionManager_winDataSource() = default;

    virtual types::NetworkInterface currentNetworkInterface() = 0;
    virtual QString currentNetworkInterfaceGuid() = 0;
    virtual bool isNetworkUnidentified(const QString &interfaceGuid) = 0;
    virtual bool isOnline() = 0;
    virtual std::optional<QString> networkIdFromInterfaceGuid(const QString &interfaceGuid) = 0;
    virtual void refreshNetworkInterfaces() = 0;
};
