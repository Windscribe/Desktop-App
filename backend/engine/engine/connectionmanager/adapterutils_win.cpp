#include "adapterutils_win.h"
#include <QProcess>
#include "Utils/logger.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include "Engine/Helper/helper_win.h"

void AdapterUtils_win::resetAdapter(IHelper *helper, const QString &tapName)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_win);
    qCDebug(LOG_BASIC) << "Reset TAP adapter:" << tapName;
    helper_win->executeResetTap(tapName);
}

AdapterGatewayInfo AdapterUtils_win::getWindscribeConnectedAdapterInfo()
{
    return getAdapterInfo(false, 0);
}

AdapterGatewayInfo AdapterUtils_win::getDefaultAdapterInfo()
{
    AdapterGatewayInfo info;
    DWORD  bestIfIndex;
    IPAddr ipAddr = htonl(0x08080808); // "8.8.8.8" is well-knowon Google DNS Server IP.
    if (GetBestInterface(ipAddr, &bestIfIndex) == NO_ERROR)
    {
        info = getAdapterInfo(true, bestIfIndex);
    }
    return info;
}

AdapterGatewayInfo AdapterUtils_win::getAdapterInfo(bool byIfIndex, unsigned long ifIndex)
{
    ULONG sz = sizeof(IP_ADAPTER_ADDRESSES_LH) * 32;
    QByteArray arr(sz, Qt::Uninitialized);

    ULONG ret = GetAdaptersAddresses(AF_INET, 0, NULL, (PIP_ADAPTER_ADDRESSES)arr.data(), &sz);
    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        arr.resize(sz);
        ret = GetAdaptersAddresses(AF_INET, 0, NULL, (PIP_ADAPTER_ADDRESSES)arr.data(), &sz);
    }

    if (ret != ERROR_SUCCESS )
    {
        return AdapterGatewayInfo();
    }

    AdapterGatewayInfo info;

    IP_ADAPTER_ADDRESSES_LH *aa = (IP_ADAPTER_ADDRESSES_LH *)arr.data();
    while (aa)
    {
        if ( (byIfIndex && aa->OperStatus == IfOperStatusUp && aa->IfIndex == ifIndex) ||
            (!byIfIndex && aa->OperStatus == IfOperStatusUp && wcsstr(aa->Description, L"Windscribe") != 0) )
        {
            info.setIfIndex(aa->IfIndex);
            info.setAdapterName(QString::fromUtf16((const ushort *)aa->Description));

            QString ip, gateway;
            getAdapterIpAndGateway(aa->IfIndex, ip, gateway);

            info.setAdapterIp(ip);
            info.setGateway(gateway);

            IP_ADAPTER_DNS_SERVER_ADDRESS_XP *dns_address = aa->FirstDnsServerAddress;
            while (dns_address)
            {
                char buf[64];
                DWORD lenStr = sizeof(buf);
                if (WSAAddressToStringA(dns_address->Address.lpSockaddr, dns_address->Address.iSockaddrLength, nullptr, buf, &lenStr) == NULL)
                {
                    info.addDnsServer(buf);
                }

                dns_address = dns_address->Next;
            }
            break;
        }

        aa = aa->Next;
    }

    return info;

}

void AdapterUtils_win::getAdapterIpAndGateway(unsigned long ifIndex, QString &outIp, QString &outGateway)
{
    ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO) * 32;
    QByteArray pAdapterInfo(ulAdapterInfoSize, Qt::Uninitialized);

    DWORD ret = GetAdaptersInfo((IP_ADAPTER_INFO *)pAdapterInfo.data(), &ulAdapterInfoSize);
    if (ret == ERROR_BUFFER_OVERFLOW ) // out of buff
    {
        pAdapterInfo.resize(ulAdapterInfoSize);
        ret = GetAdaptersInfo((IP_ADAPTER_INFO *)pAdapterInfo.data(), &ulAdapterInfoSize);
    }
    if( ret == ERROR_SUCCESS )
    {
        IP_ADAPTER_INFO *ai = (IP_ADAPTER_INFO *)pAdapterInfo.data();

        do
        {
            if (ai->Index == ifIndex)
            {
                outIp = QString(ai->IpAddressList.IpAddress.String);
                outGateway = QString(ai->GatewayList.IpAddress.String);
                return;
            }
            ai = ai->Next;
        } while(ai);
    }
}

