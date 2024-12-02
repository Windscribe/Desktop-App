#include "dnsinfo_win.h"

#include <windows.h>
#include <Iphlpapi.h>
#include <QByteArray>

#include "utils/log/categories.h"

namespace DnsInfo_win
{

void outputDebugDnsInfo()
{
    qCInfo(LOG_BASIC) << "=== DNS Configuration begin ===";

    ULONG bufSize = sizeof(IP_ADAPTER_INFO) * 32;
    QByteArray adapterInfo(bufSize, Qt::Uninitialized);

    DWORD result = ::GetAdaptersInfo((PIP_ADAPTER_INFO)adapterInfo.data(), &bufSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        adapterInfo.resize(bufSize);
        result = ::GetAdaptersInfo((PIP_ADAPTER_INFO)adapterInfo.data(), &bufSize);
    }

    if (result != NO_ERROR) {
        if (result == ERROR_NO_DATA) {
            qCWarning(LOG_BASIC) << "No adapters information is available";
        }
        else {
            qCCritical(LOG_BASIC) << "GetAdaptersInfo failed:" << result;
        }
    }
    else {
        PIP_ADAPTER_INFO pAdapter = (PIP_ADAPTER_INFO)adapterInfo.data();
        do {
            if ((pAdapter->Type == MIB_IF_TYPE_ETHERNET) || (pAdapter->Type == IF_TYPE_IEEE80211)) {
                bufSize = sizeof(IP_PER_ADAPTER_INFO) * 32;
                QByteArray perAdapterInfo(bufSize, Qt::Uninitialized);

                result = ::GetPerAdapterInfo(pAdapter->Index, (PIP_PER_ADAPTER_INFO)perAdapterInfo.data(), &bufSize);
                if (result == ERROR_BUFFER_OVERFLOW ) {
                    perAdapterInfo.resize(bufSize);
                    result = ::GetPerAdapterInfo(pAdapter->Index, (PIP_PER_ADAPTER_INFO)perAdapterInfo.data(), &bufSize);
                }

                if (result == NO_ERROR) {
                    PIP_PER_ADAPTER_INFO pai = (PIP_PER_ADAPTER_INFO)perAdapterInfo.data();
                    std::string dnsIps = pai->DnsServerList.IpAddress.String;
                    PIP_ADDR_STRING pAddrStr = pai->DnsServerList.Next;
                    while (pAddrStr) {
                        dnsIps += ", ";
                        dnsIps += pAddrStr->IpAddress.String;
                        pAddrStr = pAddrStr->Next;
                    }
                    qCInfo(LOG_BASIC) << pAdapter->Description << "(" << QString::fromStdString(dnsIps) << ")";
                }
                else {
                    qCWarning(LOG_BASIC) << "GetPerAdapterInfo (" << pAdapter->Description << ") failed:" << result;
                }
            }
            pAdapter = pAdapter->Next;
        } while (pAdapter);
    }

    qCInfo(LOG_BASIC) << "=== DNS Configuration end ===";
}

}
