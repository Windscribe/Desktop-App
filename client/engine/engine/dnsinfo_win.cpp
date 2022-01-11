#include "dnsinfo_win.h"

#include <windows.h>
#include <Iphlpapi.h>
#include <QByteArray>
#include "utils/logger.h"

void DnsInfo_win::outputDebugDnsInfo()
{
    qCDebug(LOG_BASIC) << "=== DNS Configuration begin ===";

    PIP_ADDR_STRING pAddrStr;

    ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
    QByteArray pAdapterInfo(ulAdapterInfoSize, Qt::Uninitialized);

    if( GetAdaptersInfo((IP_ADAPTER_INFO *)pAdapterInfo.data(), &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW ) // out of buff
    {
        pAdapterInfo.resize(ulAdapterInfoSize);
    }

    if( GetAdaptersInfo((IP_ADAPTER_INFO *)pAdapterInfo.data(), &ulAdapterInfoSize) == ERROR_SUCCESS )
    {
        IP_ADAPTER_INFO *ai = (IP_ADAPTER_INFO *)pAdapterInfo.data();

        do
        {
            if ((ai->Type == MIB_IF_TYPE_ETHERNET) 	// If type is etherent
                    || (ai->Type == IF_TYPE_IEEE80211))   // radio
            {
                ULONG ulPerAdapterInfoSize = sizeof(IP_PER_ADAPTER_INFO);
                QByteArray pPerAdapterInfo(ulPerAdapterInfoSize, Qt::Uninitialized);

                if( GetPerAdapterInfo(
                            ai->Index,
                            (IP_PER_ADAPTER_INFO*)pPerAdapterInfo.data(),
                            &ulPerAdapterInfoSize) == ERROR_BUFFER_OVERFLOW ) // out of buff
                {
                    pPerAdapterInfo.resize(ulPerAdapterInfoSize);
                }

                DWORD dwRet;
                std::string dnsIps;
                if((dwRet = GetPerAdapterInfo(
                            ai->Index,
                            (IP_PER_ADAPTER_INFO*)pPerAdapterInfo.data(),
                            &ulPerAdapterInfoSize)) == ERROR_SUCCESS)
                {
                    IP_PER_ADAPTER_INFO *pai = (IP_PER_ADAPTER_INFO*)pPerAdapterInfo.data();
                    pAddrStr   =   pai->DnsServerList.Next;
                    dnsIps += pai->DnsServerList.IpAddress.String;
                    while(pAddrStr)
                    {
                        dnsIps += ",";
                        dnsIps += pAddrStr->IpAddress.String;
                        pAddrStr   =   pAddrStr->Next;
                    }
                }
                qCDebug(LOG_BASIC) << ai->Description << "(" << QString::fromStdString(dnsIps) << ")";
            }
            ai = ai->Next;
        } while(ai);
    }
    qCDebug(LOG_BASIC) << "=== DNS Configuration end ===";
}
