#include "adaptergatewayinfo.h"
#include "utils/logger.h"
#include "utils/macutils.h"
#include "utils/linuxutils.h"

#ifdef Q_OS_WIN
    #include "adapterutils_win.h"
#endif

const int typeIdAdapterGatewayInfo = qRegisterMetaType<AdapterGatewayInfo>("AdapterGatewayInfo");


AdapterGatewayInfo AdapterGatewayInfo::detectAndCreateDefaultAdaperInfo()
{
    AdapterGatewayInfo cai;

#ifdef Q_OS_WIN
    cai = AdapterUtils_win::getDefaultAdapterInfo();
#elif defined Q_OS_MAC
    MacUtils::getDefaultRoute(cai.gateway_, cai.adapterName_);
    cai.adapterIp_ = MacUtils::ipAddressByInterfaceName(cai.adapterName_);
    cai.dnsServers_ = MacUtils::getDnsServersForInterface(cai.adapterName_);
#elif defined Q_OS_LINUX
    LinuxUtils::getDefaultRoute(cai.gateway_, cai.adapterName_);
    // todo: for split tunneling
    // cai.adapterIp_ =
    //cai.dnsServers_ =
#endif

    return cai;
}

AdapterGatewayInfo::AdapterGatewayInfo() : ifIndex_(0)
{
}

bool AdapterGatewayInfo::isEmpty() const
{
    return adapterName_.isEmpty();
}

QString AdapterGatewayInfo::makeLogString()
{
    return QString("adapter name = %1; adapter IP = %2; gateway IP = %3; remote IP = %4; dns = (%5); ifIndex = %6")
            .arg(adapterName_).arg(adapterIp_).arg(gateway_).arg(remoteIp_).arg(dnsServers_.join(',')).arg(ifIndex_ != 0 ? QString::number(ifIndex_) : "not detected");
}
