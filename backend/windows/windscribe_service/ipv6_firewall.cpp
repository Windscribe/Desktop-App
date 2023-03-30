#include "all_headers.h"

#include "ipv6_firewall.h"
#include "logger.h"
#include "utils.h"
#include "adapters_info.h"

#define FIREWALL_SUBLAYER_IPV6_NAMEW L"WindscribeIPV6FirewallSublayer"
#define UUID_LAYER_IPV6 L"8b03e426-56a7-4669-a985-706406675e68"

Ipv6Firewall::Ipv6Firewall(FwpmWrapper &fwmpWrapper): fwmpWrapper_(fwmpWrapper)
{
    UuidFromString((RPC_WSTR)UUID_LAYER_IPV6, &subLayerGUID_);
}

Ipv6Firewall::~Ipv6Firewall()
{
    enableIPv6();
}

void Ipv6Firewall::enableIPv6()
{
    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    if (Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_)) {
        Logger::instance().out(L"Ipv6Firewall::enableIPv6(), all filters deleted");
    } else {
        Logger::instance().out(L"Ipv6Firewall::enableIPv6(), failed delete filters");
    }

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();
}

void Ipv6Firewall::disableIPv6()
{
    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    FWPM_SUBLAYER0 subLayer = { 0 };

    subLayer.subLayerKey = subLayerGUID_;
    subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
    subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
    subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
    subLayer.weight = 0xFFFF;

    DWORD dwFwAPiRetCode = FwpmSubLayerAdd0(hEngine, &subLayer, NULL);
    if (dwFwAPiRetCode != ERROR_SUCCESS && dwFwAPiRetCode != FWP_E_ALREADY_EXISTS) {
        Logger::instance().out(L"Ipv6Firewall::disableIPv6(), FwpmSubLayerAdd0 failed");
        fwmpWrapper_.endTransaction();
        fwmpWrapper_.unlock();
        return;
    }

    addFilters(hEngine);

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();
}

void Ipv6Firewall::addFilters(HANDLE engineHandle)
{
    AdaptersInfo ai;
    std::vector<NET_IFINDEX> taps = ai.getTAPAdapters();

    // add block filter for all IPv6
    bool ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 0, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW);
    if (!ret) {
        Logger::instance().out(L"Ipv6Firewall::addFilters(), FwpmFilterAdd0 failed");
    }

    // add permit filter IPv6 for TAP-adapters
    for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it) {
        NET_LUID luid;
        ConvertInterfaceIndexToLuid(*it, &luid);

        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 0, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, &luid);
        if (!ret) {
            Logger::instance().out(L"Ipv6Firewall::addFilters(), add permit filter failed");
        }
    }

    // add permit filter for ipv6 localhost address
    const std::vector<Ip6AddressAndPrefix> localhost = Ip6AddressAndPrefix::fromVector({L"::1/128"});
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, nullptr, &localhost);
    if (!ret) {
        Logger::instance().out(L"Could not add IPv6 LAN traffic allow filter");
        return;
    }
}