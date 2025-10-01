#pragma once

#include <QString>
#include "adaptergatewayinfo.h"

// Windows network util functions for get info about adapters
class AdapterUtils_win
{
public:
    static AdapterGatewayInfo getDefaultAdapterInfo();
    static AdapterGatewayInfo getConnectedAdapterInfo(const QString &adapterIdentifier);

private:
    // if byIfIndex == true, then find by ifIndex
    // overwise by "Windscribe" description
    static AdapterGatewayInfo getAdapterInfo(bool byIfIndex, unsigned long ifIndex, const QString &adapterIdentifier);
    static void getAdapterIpAndGateway(unsigned long ifIndex, QString &outIp, QString &outGateway);
};
