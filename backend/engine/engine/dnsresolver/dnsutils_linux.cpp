#include "dnsutils.h"
#include "utils/logger.h"

#include <QProcess>


namespace DnsUtils
{

std::vector<std::wstring> getOSDefaultDnsServers()
{
    std::vector<std::wstring> dnsServers;
    //todo linux
    return dnsServers;
}

}
