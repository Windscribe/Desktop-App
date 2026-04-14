#include "ws_branding.h"
#include "adapters_info.h"

#include <ws2tcpip.h>
#include <spdlog/spdlog.h>

AdaptersInfo::AdaptersInfo()
{
    ULONG ulAdapterInfoSize = 4096;
    adapterInfoBuffer_.reset(new unsigned char[ulAdapterInfoSize]);

    DWORD result = ::GetAdaptersAddresses(AF_UNSPEC, NULL, NULL, (PIP_ADAPTER_ADDRESSES)adapterInfoBuffer_.get(), &ulAdapterInfoSize);

    if (result == ERROR_BUFFER_OVERFLOW)
    {
        adapterInfoBuffer_.reset(new unsigned char[ulAdapterInfoSize]);
        result = ::GetAdaptersAddresses(AF_UNSPEC, NULL, NULL, (PIP_ADAPTER_ADDRESSES)adapterInfoBuffer_.get(), &ulAdapterInfoSize);
    }

    if (result == ERROR_SUCCESS)
    {
        pAdapterInfo_ = (PIP_ADAPTER_ADDRESSES)adapterInfoBuffer_.get();
    }
    else
    {
        adapterInfoBuffer_.reset();
        spdlog::error(L"AdaptersInfo - GetAdaptersAddresses failed {}", ::GetLastError());
    }
}


bool AdaptersInfo::isAppVpnAdapter(NET_IFINDEX index) const
{
    PIP_ADAPTER_ADDRESSES ai = pAdapterInfo_;
    while (ai)
    {
        if (ai->IfIndex == index)
        {
            return isAppVpnAdapter(ai);
        }
        ai = ai->Next;
    }

    return false;
}

bool AdaptersInfo::getIkev2AdapterInfo(NET_IFINDEX &outIfIndex, std::wstring &outIp)
{
    outIfIndex = NET_IFINDEX_UNSPECIFIED;
    outIp.clear();

    PIP_ADAPTER_ADDRESSES ai = pAdapterInfo_;
    while (ai)
    {
        if (wcsstr(ai->Description, WS_WIN_IKEV2_CONNECTION_NAME_W) != 0)
        {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = ai->FirstUnicastAddress;
            while (pUnicast)
            {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                {
                    wchar_t szBuf[256];
                    DWORD bufSize = 256;
                    int ret = ::WSAAddressToString(pUnicast->Address.lpSockaddr,
                                                   pUnicast->Address.iSockaddrLength,
                                                   NULL, szBuf, &bufSize);
                    if (ret == 0) {
                        outIp = std::wstring(szBuf);
                    }
                    else {
                        spdlog::error("AdaptersInfo - WSAAddressToString failed {}", ::WSAGetLastError());
                    }

                    break;
                }

                pUnicast = pUnicast->Next;
            }

            if (outIp.empty()) {
                spdlog::error("AdaptersInfo::getIkev2AdapterInfo - failed to determine IPv4 address");
            }
            else {
                outIfIndex = ai->IfIndex;
            }

            break;
        }
        ai = ai->Next;
    }

    return (outIfIndex != NET_IFINDEX_UNSPECIFIED);
}

std::vector<NET_IFINDEX> AdaptersInfo::getTAPAdapters()
{
    std::vector<NET_IFINDEX> list;

    PIP_ADAPTER_ADDRESSES ai = pAdapterInfo_;
    while (ai)
    {
        if (isAppVpnAdapter(ai)) {
            list.push_back(ai->IfIndex);
        }
        ai = ai->Next;
    }

    return list;
}

std::vector<std::string> AdaptersInfo::getAdapterAddresses(NET_IFINDEX idx)
{
    std::vector<std::string> list;

    PIP_ADAPTER_ADDRESSES ai = pAdapterInfo_;
    while (ai) {
        if (idx == ai->IfIndex) {
            PIP_ADAPTER_UNICAST_ADDRESS addr = ai->FirstUnicastAddress;
            while (addr) {
                char str[16] = { 0 };

                if (addr->Address.lpSockaddr->sa_family == AF_INET) {
                    inet_ntop(AF_INET, &(((struct sockaddr_in *)addr->Address.lpSockaddr)->sin_addr), str, 16);
                    list.push_back(std::string(str));
                }
                addr = addr->Next;
            }
        }
        ai = ai->Next;
    }

    return list;
}

bool AdaptersInfo::isAppVpnAdapter(PIP_ADAPTER_ADDRESSES ai) const
{
    // Warning: we control the FriendlyName of the wireguard-nt adapter, but not the Description.
    return (wcsstr(ai->Description, WS_PRODUCT_NAME_W) != 0) || (wcsstr(ai->FriendlyName, WS_PRODUCT_NAME_W) != 0);
}
