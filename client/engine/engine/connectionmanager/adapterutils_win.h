#ifndef ADAPTER_UTILS_WIN_H
#define ADAPTER_UTILS_WIN_H

#include <QString>
#include "adaptergatewayinfo.h"

class IHelper;

// Windows network util functions for get info about adapters
class AdapterUtils_win
{
public:
    static void resetAdapter(IHelper *helper, const QString &tapName);

    static AdapterGatewayInfo getWindscribeConnectedAdapterInfo();
    static AdapterGatewayInfo getDefaultAdapterInfo();
    static AdapterGatewayInfo getWireguardConnectedAdapterInfo(const QString &serviceIdentifier);

private:
    // if byIfIndex == true, then find by ifIndex
    // overwise by "Windscribe" description
    static AdapterGatewayInfo getAdapterInfo(bool byIfIndex, unsigned long ifIndex, const QString &serviceIdentifier);
    static void getAdapterIpAndGateway(unsigned long ifIndex, QString &outIp, QString &outGateway);
};

#endif // ADAPTER_UTILS_WIN_H
