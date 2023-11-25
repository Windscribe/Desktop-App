#include "all_headers.h"

#include "dns_firewall.h"
#include "logger.h"
#include "utils.h"
#include "ip_address/ip4_address_and_mask.h"

#define FIREWALL_SUBLAYER_DNS_NAMEW L"WindscribeDnsFirewallSublayer"
#define UUID_LAYER_DNS L"7e4a5678-bc70-45e8-8674-21a8e610fd02"


DnsFirewall::DnsFirewall(FwpmWrapper &fwmpWrapper) : fwmpWrapper_(fwmpWrapper), bCurrentState_(false)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    UuidFromString((RPC_WSTR)UUID_LAYER_DNS, &subLayerGUID_);
}

void DnsFirewall::release()
{
    disable();
    WSACleanup();
}

void DnsFirewall::disable()
{
    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();

    fwmpWrapper_.beginTransaction();
    bCurrentState_ = false;

    if (Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_)) {
        Logger::instance().out(L"DnsFirewall::disable(), all filters deleted");
    } else {
        Logger::instance().out(L"DnsFirewall::disable(), failed delete filters");
    }

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();
}

void DnsFirewall::enable()
{
    // if already enabled, first remove previous filter
    if (bCurrentState_)
    {
        disable();
    }

    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    FWPM_SUBLAYER0 subLayer = { 0 };

    subLayer.subLayerKey = subLayerGUID_;
    subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_DNS_NAMEW;
    subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_DNS_NAMEW;
    subLayer.flags = 0;
    subLayer.weight = 0xFFFF;

    DWORD dwFwAPiRetCode = FwpmSubLayerAdd0(hEngine, &subLayer, NULL);
    if (dwFwAPiRetCode != ERROR_SUCCESS && dwFwAPiRetCode != FWP_E_ALREADY_EXISTS)
    {
        Logger::instance().out(L"DnsFirewall::enable(), FwpmSubLayerAdd0 failed");
        fwmpWrapper_.endTransaction();
        fwmpWrapper_.unlock();
        return;
    }

    addFilters(hEngine);

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();
    bCurrentState_ = true;
}

void DnsFirewall::setExcludeIps(const std::vector<std::wstring>& ips)
{
    excludeIps_ = std::unordered_set<std::wstring>(ips.cbegin(), ips.cend());
}

void DnsFirewall::addFilters(HANDLE engineHandle)
{
    std::vector<std::wstring> dnsServers = getDnsServers();
    // add block filter for DNS ips for protocol UDP port 53
    for (size_t i = 0; i < dnsServers.size(); ++i) {
        if (excludeIps_.find(dnsServers[i]) != excludeIps_.cend()) {
            Logger::instance().out(L"DnsFirewall::addFilters(), ip excluded: ", dnsServers[i].c_str());
            continue;
        }

        const std::vector<Ip4AddressAndMask> addr = Ip4AddressAndMask::fromVector({dnsServers[i].c_str()});
        // Rule weight must be greater than the LAN allow rule (4)
        bool ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 6, subLayerGUID_, (wchar_t*)FIREWALL_SUBLAYER_DNS_NAMEW, nullptr, &addr, 0, 53, nullptr, false);
        if (!ret) {
            Logger::instance().out(L"Could not add DNS filter for %s", dnsServers[i].c_str());
        } else {
            Logger::instance().out(L"Added DNS filter for %s", dnsServers[i].c_str());
        }
    }
}

std::vector<std::wstring> DnsFirewall::getDnsServers()
{
    std::vector<std::wstring> dnsServers;

    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    ULONG iterations = 0;
    const int MAX_TRIES = 5;
    std::vector<unsigned char> arr;
    arr.resize(1);

    outBufLen = static_cast<ULONG>(arr.size());
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
        if ((wcsstr(pCurrAddresses->Description, L"Windscribe") == 0) &&
            (wcsstr(pCurrAddresses->FriendlyName, L"Windscribe") == 0))
        {
            PIP_ADAPTER_DNS_SERVER_ADDRESS dnsServerAddress = pCurrAddresses->FirstDnsServerAddress;
            while (dnsServerAddress)
            {
                if (dnsServerAddress->Address.lpSockaddr->sa_family == AF_INET)
                {
                    const int BUF_LEN = 256;
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

    return dnsServers;
}
