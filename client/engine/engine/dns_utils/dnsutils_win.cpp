#include <QtGlobal>
#include "dnsutils.h"

#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <windows.h>
    #include <iphlpapi.h>
#endif
#include "utils/ws_assert.h"

namespace DnsUtils
{

std::vector<std::wstring> getOSDefaultDnsServers()
{
    std::vector<std::wstring> dnsServers;

#ifdef Q_OS_WIN
    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    ULONG iterations = 0;
    const int MAX_TRIES = 5;
    std::vector<unsigned char> arr;
    arr.resize(1);

    outBufLen = arr.size();
    do {
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, NULL, NULL, (PIP_ADAPTER_ADDRESSES)&arr[0], &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW)
        {
            arr.resize(outBufLen);
        }
        else
        {
            break;
        }
        iterations++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iterations < MAX_TRIES));

    if (dwRetVal != NO_ERROR)
    {
        return dnsServers;
    }

    PIP_ADAPTER_ADDRESSES pCurrAddresses = (PIP_ADAPTER_ADDRESSES)&arr[0];
    while (pCurrAddresses)
    {
        // Warning: we control the FriendlyName of the wireguard-nt adapter, but not the Description.
        if ((wcsstr(pCurrAddresses->Description, L"Windscribe") == 0) && // ignore Windscribe TAP and Windscribe Ikev2 adapters
            (wcsstr(pCurrAddresses->FriendlyName, L"Windscribe") == 0)) // ignore Windscribe wireguard-nt tunnel
        {
            PIP_ADAPTER_DNS_SERVER_ADDRESS dnsServerAddress = pCurrAddresses->FirstDnsServerAddress;
            while (dnsServerAddress)
            {
                const int BUF_LEN = 256;
                if (dnsServerAddress->Address.lpSockaddr->sa_family == AF_INET)
                {
                    WCHAR szBuf[BUF_LEN];
                    DWORD bufSize = BUF_LEN;
                    int ret = WSAAddressToString(dnsServerAddress->Address.lpSockaddr, dnsServerAddress->Address.iSockaddrLength, NULL, szBuf, &bufSize);
                    if (ret == 0)  // ok
                    {
                        dnsServers.push_back(std::wstring(szBuf));
                    }
                }
                dnsServerAddress = dnsServerAddress->Next;
            }
        }

        pCurrAddresses = pCurrAddresses->Next;
    }
#else
    WS_ASSERT(false);
#endif

    return dnsServers;
}

}
