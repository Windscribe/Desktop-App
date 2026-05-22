#include "ws_branding.h"
#include <Windows.h>
#include <Fwpmu.h>
#include <initguid.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <spdlog/spdlog.h>

#include "callout_filter.h"
#include "../utils.h"
#include "types/ipaddress.h"

DEFINE_GUID(
    BIND_CALLOUT_GUID,
    0x5DF29179,
    0x344E,
    0x4F9C,
    0xA4, 0x5D, 0xC3, 0x0F, 0x95, 0x9B, 0x01, 0x2D
);

DEFINE_GUID(
    TCP_CALLOUT_GUID,
    0xB53C4ADE,
    0x7A35,
    0x4A63,
    0xB6, 0x90, 0xD9, 0x6B, 0xD4, 0x26, 0x23, 0x03
);

DEFINE_GUID(
    BIND_CALLOUT_V6_GUID,
    0x6EF39280,
    0x455F,
    0x4A9D,
    0xB5, 0x6E, 0xD4, 0x1F, 0xA6, 0xAC, 0x02, 0x36
);

DEFINE_GUID(
    TCP_CALLOUT_V6_GUID,
    0xC64D5BEF,
    0x8B46,
    0x4B74,
    0xC7, 0xA1, 0xEA, 0x7C, 0xE5, 0x37, 0x34, 0x14
);

DEFINE_GUID(
    CALLOUT_PROVIDER_CONTEXT_IP_GUID,
    0x9DCD5EC9,
    0x56A2,
    0x42D3,
    0xA1, 0x8A, 0x52, 0xA7, 0x99, 0xA1, 0xD4, 0x62
);

DEFINE_GUID(
    CALLOUT_PROVIDER_CONTEXT_IPV6_GUID,
    0xAEE46FCA,
    0x67B3,
    0x43E4,
    0xB2, 0x9B, 0x63, 0xA8, 0x8A, 0xB2, 0xE5, 0x73
);

DEFINE_GUID(
    SUBLAYER_CALLOUT_GUID,
    0xB32BE898,
    0x4077,
    0x45E2,
    0xBC, 0x77, 0x1F, 0xA4, 0x6E, 0x99, 0x4A, 0x4B
);


#pragma pack(push,1)
typedef struct CALLOUT_DATA_
{
    UINT32 localIp;
    UINT32 vpnIp;
    UINT8 isExclude;

    // this is a list of addresses represented by pairs of addreses and masks:
    // excludeAddresses[0] -> Address_0; excludeAddresses[1] -> Mask_0
    // .....
    // excludeAddresses[cntExcludeAddresses - 2] -> Address_last; excludeAddresses[cntExcludeAddresses - 1] -> Mask_last
    UINT16 cntExcludeAddresses;
    UINT32 excludeAddresses[1];

} CALLOUT_DATA;

typedef struct CALLOUT_DATA_V6_
{
    UINT8 localIp[16];
    UINT8 vpnIp[16];
    UINT8 isExclude;

    // Pairs of 16-byte entries: address then mask (each IN6_ADDR-sized).
    // cntExcludeAddresses is the total number of 16-byte entries (pairs * 2).
    UINT16 cntExcludeAddresses;
    UINT8 excludeAddresses[1];

} CALLOUT_DATA_V6;
#pragma pack(pop)

static void prefixToMaskV6(UINT8 prefix, UINT8 mask[16])
{
    memset(mask, 0, 16);
    for (int i = 0; i < 16; i++) {
        if (prefix >= 8) {
            mask[i] = 0xFF;
            prefix -= 8;
        } else {
            mask[i] = static_cast<UINT8>(0xFF << (8 - prefix));
            prefix = 0;
        }
    }
}


CalloutFilter::CalloutFilter(FwpmWrapper &fwmpWrapper): fwmpWrapper_(fwmpWrapper), isEnabled_(false), prevLocalIp_(0), prevVpnIp_(0)
{
}

void CalloutFilter::enable(UINT32 localIp, UINT32 vpnIp,
                           const types::IpAddress &localIpV6, const types::IpAddress &vpnIpV6,
                           const AppsIds &appsIds, const AppsIds &ctrldAppId,
                           bool isExclude, bool isAllowLanTraffic,
                           unsigned long vpnIfIndex)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    spdlog::debug("CalloutFilter::enable()");

    if (isEnabled_ &&
        localIp == prevLocalIp_ &&
        vpnIp == prevVpnIp_ &&
        localIpV6 == prevLocalIpV6_ &&
        vpnIpV6 == prevVpnIpV6_ &&
        appsIds == appsIds_ &&
        isExclude == prevIsExclude_ &&
        isAllowLanTraffic == prevIsAllowLanTraffic_ &&
        vpnIfIndex == prevVpnIfIndex_)
    {
        return;
    }

    if (isEnabled_) {
        spdlog::debug("CalloutFilter: disable before enable");
        disable();
    }

    appsIds_ = appsIds;
    prevLocalIp_ = localIp;
    prevVpnIp_ = vpnIp;
    prevLocalIpV6_ = localIpV6;
    prevVpnIpV6_ = vpnIpV6;
    prevIsExclude_ = isExclude;
    prevIsAllowLanTraffic_ = isAllowLanTraffic;
    prevVpnIfIndex_ = vpnIfIndex;

    bool withV6 = localIpV6.isValid() && localIpV6.isV6() && vpnIpV6.isValid() && vpnIpV6.isV6();

    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    bool success = true;

    if (!addProviderContext(hEngine, CALLOUT_PROVIDER_CONTEXT_IP_GUID, localIp, vpnIp, isExclude)) {
        spdlog::error("CalloutFilter::enable(), addProviderContext failed");
        success = false;
    }

    if (success && withV6) {
        if (!addProviderContextV6(hEngine, CALLOUT_PROVIDER_CONTEXT_IPV6_GUID, localIpV6, vpnIpV6, isExclude)) {
            spdlog::error("CalloutFilter::enable(), addProviderContextV6 failed");
            success = false;
        }
    }

    if (success && !addCallouts(hEngine, withV6)) {
        spdlog::error("CalloutFilter::enable(), FwpmCalloutAdd failed");
        success = false;
    }

    if (success && !addSubLayer(hEngine)) {
        spdlog::error("CalloutFilter::enable(), addSubLayer failed");
        success = false;
    }

    if (success && !addFilters(hEngine, !isExclude && isAllowLanTraffic, withV6, appsIds, ctrldAppId, isExclude, vpnIfIndex)) {
        spdlog::error("CalloutFilter::enable(), addFilters failed");
        success = false;
    }

    // v6 anti-leak BLOCK is added inside addFilters() with the same gating condition. If
    // it succeeded, capture the VPN LUID and reapply any whitelist IPs that were pushed
    // before this enable() ran (HostnamesManager pushes them via setV6WhitelistIps()
    // immediately on its own enable(), which fires before CalloutFilter::enable() in
    // SplitTunneling::updateState()).
    if (success && withV6 && !isExclude && vpnIfIndex != 0 && appsIds.count() > 0) {
        NET_LUID vpnLuid = { 0 };
        if (ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(vpnIfIndex), &vpnLuid) == NO_ERROR) {
            v6AntiLeakActive_ = true;
            v6AntiLeakVpnLuid_ = vpnLuid.Value;
            applyV6WhitelistPermitFiltersLocked(hEngine);
        }
        // If LUID lookup failed, the BLOCK in addFilters() also failed to install for the
        // same reason and logged its own error; nothing more to do here.
    }

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();

    if (success) {
        isEnabled_ = true;
    }
}

void CalloutFilter::disable()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    spdlog::debug("CalloutFilter::disable()");
    removeAllFilters(fwmpWrapper_);
    isEnabled_ = false;

    // The whole sublayer was just dropped, so any tracked filter ids are already gone
    // from WFP — clear our bookkeeping so we don't try to delete stale ids on the next
    // setV6WhitelistIps(). The IP list itself is preserved: the next enable() will pick
    // it up via applyV6WhitelistPermitFiltersLocked().
    v6AntiLeakActive_ = false;
    v6AntiLeakVpnLuid_ = 0;
    v6WhitelistFilterIds_.clear();
}

void CalloutFilter::setV6WhitelistIps(const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    if (v6WhitelistIps_ == ips) {
        return;
    }
    v6WhitelistIps_ = ips;

    // Without an active anti-leak BLOCK there is nothing to bypass, so just hold the
    // list for a future enable(). This mirrors FirewallFilter::setSplitTunnelingWhitelistIps
    // which silently no-ops while split tunneling / firewall are off.
    if (!v6AntiLeakActive_) {
        return;
    }

    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    removeV6WhitelistPermitFiltersLocked(hEngine);
    applyV6WhitelistPermitFiltersLocked(hEngine);

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();
}

void CalloutFilter::applyV6WhitelistPermitFiltersLocked(HANDLE engineHandle)
{
    if (!v6AntiLeakActive_ || v6WhitelistIps_.empty()) {
        return;
    }

    // addFilterV6 internally skips ranges whose family doesn't match and turns into a
    // no-op when no v6 entries remain, so passing the dual-stack vector unfiltered is
    // safe. It does, however, pack all matching ranges into a single filter via repeated
    // FWPM_CONDITION_IP_REMOTE_ADDRESS conditions (WFP OR's same-field conditions),
    // which keeps the per-packet cost flat as the inclusive list grows.
    //
    // Weight 5 sits between the BLOCK (4) and the per-app PERMIT (6). Two PERMITs
    // arbitrate as PERMIT regardless of order, so the relative weight only matters
    // against the BLOCK — which any weight > 4 wins.
    //
    // persistent=false: SUBLAYER_CALLOUT_GUID is transient; persistent filters in a
    // transient sublayer fail to register with FWP_E_LIFETIME_MISMATCH.
    NET_LUID vpnLuid;
    vpnLuid.Value = v6AntiLeakVpnLuid_;

    bool ok = Utils::addFilterV6(engineHandle, &v6WhitelistFilterIds_, FWP_ACTION_PERMIT, 5,
                                 SUBLAYER_CALLOUT_GUID,
                                 (wchar_t *)WS_PRODUCT_NAME_W L" v6 whitelist permit on VPN",
                                 &vpnLuid, &v6WhitelistIps_, 0, 0, nullptr, false);
    if (!ok) {
        spdlog::error("CalloutFilter::applyV6WhitelistPermitFiltersLocked(), addFilterV6 failed");
    }
}

void CalloutFilter::removeV6WhitelistPermitFiltersLocked(HANDLE engineHandle)
{
    for (UINT64 id : v6WhitelistFilterIds_) {
        DWORD ret = FwpmFilterDeleteById0(engineHandle, id);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("CalloutFilter::removeV6WhitelistPermitFiltersLocked(), FwpmFilterDeleteById0 failed: {}", ret);
        }
    }
    v6WhitelistFilterIds_.clear();
}

bool CalloutFilter::addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 localIp, UINT32 vpnIp, bool isExclude)
{
    const std::vector<std::string> lanRanges = {
        "10.0.0.0/9",    // this and the following 10.x.x.x ranges represent the range 10.0.0.0 - 10.254.255.255,
        "10.128.0.0/10", // specifically excluding the reserved range of 10.255.255.0/24
        "10.192.0.0/11",
        "10.224.0.0/12",
        "10.240.0.0/13",
        "10.248.0.0/14",
        "10.252.0.0/15",
        "10.254.0.0/16",
        "127.0.0.0/8",
        "169.254.0.0/16",
        "172.16.0.0/12",
        "192.168.0.0/16",
        "224.0.0.0/4",
    };

    bool bRet = true;
    FWP_BYTE_BLOB         byteBlob = { 0 };

    // fill callout data struct
    // here is some C style code needed to fill the structure and pass it to the driver
    std::vector<unsigned char> data;
    data.resize(sizeof(CALLOUT_DATA) - sizeof(CALLOUT_DATA().excludeAddresses) + lanRanges.size() * 2 * sizeof(UINT32));
    CALLOUT_DATA *proxyData = reinterpret_cast<CALLOUT_DATA *>(data.data());

    proxyData->localIp = localIp;
    proxyData->vpnIp = vpnIp;
    proxyData->isExclude = isExclude;
    proxyData->cntExcludeAddresses = lanRanges.size() * 2;

    int ind = 0;
    for (const auto &it: lanRanges) {
        types::IpAddressRange addr(it);
        proxyData->excludeAddresses[ind++] = addr.address().ipv4HostOrder();
        proxyData->excludeAddresses[ind++] = addr.ipv4MaskHostOrder();
    }

    FWPM_PROVIDER_CONTEXT1 providerContext = { 0 };
    UINT64 providerContextId;

    providerContext.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " provider context for callout driver";
    providerContext.providerContextKey = guid;
    providerContext.type = FWPM_GENERAL_CONTEXT;

    providerContext.dataBuffer = &byteBlob;
    providerContext.dataBuffer->size = data.size();
    providerContext.dataBuffer->data = (UINT8*)data.data();

    DWORD ret = FwpmProviderContextAdd1(engineHandle, &providerContext, 0, &providerContextId);
    if (ret != ERROR_SUCCESS) {
        spdlog::error("CalloutFilter::addProviderContext(): {}", ret);
        bRet = false;
    }

    return bRet;
}

bool CalloutFilter::addProviderContextV6(HANDLE engineHandle, const GUID &guid, const types::IpAddress &localIp, const types::IpAddress &vpnIp, bool isExclude)
{
    const std::vector<std::string> lanRangesV6 = {
        "::1/128",
        "fe80::/10",
        "fc00::/7",
        "ff00::/8",
    };

    std::vector<unsigned char> data;
    data.resize(sizeof(CALLOUT_DATA_V6) - sizeof(CALLOUT_DATA_V6().excludeAddresses) + lanRangesV6.size() * 2 * 16);
    CALLOUT_DATA_V6 *proxyData = reinterpret_cast<CALLOUT_DATA_V6 *>(data.data());

    if (localIp.isValid() && localIp.isV6())
        memcpy(proxyData->localIp, localIp.bytes(), 16);
    else
        memset(proxyData->localIp, 0, 16);

    if (vpnIp.isValid() && vpnIp.isV6())
        memcpy(proxyData->vpnIp, vpnIp.bytes(), 16);
    else
        memset(proxyData->vpnIp, 0, 16);

    proxyData->isExclude = isExclude;
    proxyData->cntExcludeAddresses = static_cast<UINT16>(lanRangesV6.size() * 2);

    int ind = 0;
    for (const auto &it : lanRangesV6) {
        types::IpAddressRange addr(it);
        UINT8 mask[16];
        prefixToMaskV6(addr.prefixLength(), mask);

        // Store the address in canonical form (host bits cleared). The kernel callout compares
        // (remoteAddr & mask) against the stored address directly, so a non-canonical CIDR like
        // fe80::1/10 would otherwise never match anything. Masking once here keeps the per-packet
        // comparison cheap and removes the foot-gun for future entries.
        UINT8 canonicalAddr[16];
        const UINT8 *raw = addr.address().bytes();
        for (int b = 0; b < 16; b++) {
            canonicalAddr[b] = raw[b] & mask[b];
        }
        memcpy(&proxyData->excludeAddresses[ind * 16], canonicalAddr, 16);
        ind++;
        memcpy(&proxyData->excludeAddresses[ind * 16], mask, 16);
        ind++;
    }

    FWP_BYTE_BLOB byteBlob = { 0 };
    FWPM_PROVIDER_CONTEXT1 providerContext = { 0 };
    UINT64 providerContextId;

    providerContext.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " provider context for callout driver (IPv6)";
    providerContext.providerContextKey = guid;
    providerContext.type = FWPM_GENERAL_CONTEXT;
    providerContext.dataBuffer = &byteBlob;
    providerContext.dataBuffer->size = static_cast<UINT32>(data.size());
    providerContext.dataBuffer->data = (UINT8*)data.data();

    DWORD ret = FwpmProviderContextAdd1(engineHandle, &providerContext, 0, &providerContextId);
    if (ret != ERROR_SUCCESS) {
        spdlog::error("CalloutFilter::addProviderContextV6(): {}", ret);
        return false;
    }
    return true;
}

bool CalloutFilter::addCallouts(HANDLE engineHandle, bool withV6)
{
    // Callout for non-TCP traffic
    {
        FWPM_CALLOUT0 callout = { 0 };
        UINT32 calloutId;
        callout.calloutKey = BIND_CALLOUT_GUID;
        callout.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " split tunnel non-TCP callout";
        callout.applicableLayer = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
        callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
        DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("CalloutFilter::addCallouts(), FwpmCalloutAdd (non-TCP) failed");
            return false;
        }
    }

    // Callout for TCP traffic
    {
        FWPM_CALLOUT0 callout = { 0 };
        UINT32 calloutId;
        callout.calloutKey = TCP_CALLOUT_GUID;
        callout.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " split tunnel TCP callout";
        callout.applicableLayer = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
        callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
        DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("CalloutFilter::addCallouts(), FwpmCalloutAdd (TCP) failed");
            return false;
        }
    }

    if (withV6) {
        {
            FWPM_CALLOUT0 callout = { 0 };
            UINT32 calloutId;
            callout.calloutKey = BIND_CALLOUT_V6_GUID;
            callout.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " split tunnel non-TCP callout (IPv6)";
            callout.applicableLayer = FWPM_LAYER_ALE_BIND_REDIRECT_V6;
            callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
            DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
            if (ret != ERROR_SUCCESS) {
                spdlog::error("CalloutFilter::addCallouts(), FwpmCalloutAdd (non-TCP v6) failed");
                return false;
            }
        }

        {
            FWPM_CALLOUT0 callout = { 0 };
            UINT32 calloutId;
            callout.calloutKey = TCP_CALLOUT_V6_GUID;
            callout.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " split tunnel TCP callout (IPv6)";
            callout.applicableLayer = FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
            callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
            DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
            if (ret != ERROR_SUCCESS) {
                spdlog::error("CalloutFilter::addCallouts(), FwpmCalloutAdd (TCP v6) failed");
                return false;
            }
        }
    }

    return true;
}

bool CalloutFilter::addSubLayer(HANDLE engineHandle)
{
    FWPM_SUBLAYER subLayer = { 0 };
    subLayer.subLayerKey = SUBLAYER_CALLOUT_GUID;
    subLayer.displayData.name = (wchar_t*)WS_PRODUCT_NAME_W " sublayer for callout driver";
    subLayer.weight = 0x200;

    DWORD dwFwAPiRetCode = FwpmSubLayerAdd(engineHandle, &subLayer, NULL);
    if (dwFwAPiRetCode != ERROR_SUCCESS) {
        spdlog::error("CalloutFilter::addSubLayer(), FwpmSubLayerAdd failed");
        return false;
    }
    return true;
}

bool CalloutFilter::addFilters(HANDLE engineHandle, bool withTcpFilters, bool withV6,
                               const AppsIds &appsIds, const AppsIds &ctrldAppId,
                               bool isExclude, unsigned long vpnIfIndex)
{
    bool retValue = true;

    // All TCP-traffic for apps goes to ALE_CONNECT_REDIRECT_V4 filter
    // According to the tests, it works well and no need to make the FWPM_LAYER_ALE_BIND_REDIRECT_V4 filter for TCP traffic
    if (appsIds.count() > 0) {
        std::vector<FWPM_FILTER_CONDITION> conditions;
        conditions.reserve(appsIds.count() + 1);

        // Must match one of the apps
        for (size_t i = 0; i < appsIds.count(); ++i) {
            FWPM_FILTER_CONDITION condition;
            condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
            condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)appsIds.getAppId(i);
            conditions.push_back(condition);
        }

        // must match TCP
        {
            FWPM_FILTER_CONDITION condition;
            condition.fieldKey = FWPM_CONDITION_IP_PROTOCOL;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT8;
            condition.conditionValue.uint8 = IPPROTO_TCP;
            conditions.push_back(condition);
        }

        FWPM_FILTER filter = { 0 };
        filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
        filter.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
        filter.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W " TCP filter for callout driver";
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x00;
        filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
        filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
        filter.numFilterConditions = static_cast<UINT32>(conditions.size());
        filter.filterCondition = &conditions[0];
        filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
        filter.action.calloutKey = TCP_CALLOUT_GUID;

        UINT64 filterId;
        DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
        retValue = (ret == ERROR_SUCCESS);
        if (!retValue) {
            spdlog::error("CalloutFilter::addFilter(), TCP redirect filter failed: {}", ret);
            return retValue;
        }
    }

    // All other non TCP-traffic for apps goes to ALE_BIND_REDIRECT_V4 filter
    if (appsIds.count() > 0) {
        std::vector<FWPM_FILTER_CONDITION> conditions;
        conditions.reserve(appsIds.count());
        for (size_t i = 0; i < appsIds.count(); ++i) {
            FWPM_FILTER_CONDITION condition;
            condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
            condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)appsIds.getAppId(i);
            conditions.push_back(condition);
        }

        // non TCP-traffic
        {
            FWPM_FILTER_CONDITION condition;
            condition.fieldKey = FWPM_CONDITION_IP_PROTOCOL;
            condition.matchType = FWP_MATCH_NOT_EQUAL;
            condition.conditionValue.type = FWP_UINT8;
            condition.conditionValue.uint8 = IPPROTO_TCP;
            conditions.push_back(condition);
        }

        FWPM_FILTER filter = { 0 };
        filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
        filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
        filter.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W " bind filter for callout driver";
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x00;
        filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
        filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
        filter.numFilterConditions = static_cast<UINT32>(conditions.size());
        filter.filterCondition = &conditions[0];
        filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
        filter.action.calloutKey = BIND_CALLOUT_GUID;

        UINT64 filterId;
        DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
        retValue = (ret == ERROR_SUCCESS);
        if (!retValue) {
            spdlog::error("CalloutFilter::addFilter(), bind filter failed: {}", ret);
            return retValue;
        }
    }


    // All traffic for ctrld app goes to ALE_CONNECT_REDIRECT_V4 filter
    if (ctrldAppId.count() > 0) {
        std::vector<FWPM_FILTER_CONDITION> conditions;
        conditions.reserve(ctrldAppId.count());

        for (size_t i = 0; i < ctrldAppId.count(); ++i) {
            FWPM_FILTER_CONDITION condition;
            condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
            condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)ctrldAppId.getAppId(i);
            conditions.push_back(condition);
        }

        FWPM_FILTER filter = { 0 };
        filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
        filter.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
        filter.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W " ctrld filter for callout driver";
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x00;
        filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
        filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
        filter.numFilterConditions = static_cast<UINT32>(conditions.size());
        filter.filterCondition = &conditions[0];
        filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
        filter.action.calloutKey = TCP_CALLOUT_GUID;

        UINT64 filterId;
        DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
        retValue = (ret == ERROR_SUCCESS);
        if (!retValue) {
            spdlog::error("CalloutFilter::addFilter(), ctrld redirect filter failed: {}", ret);
            return retValue;
        }
    }

    // --- IPv6 filters ---
    if (withV6) {
        // V6 TCP filter for apps → ALE_CONNECT_REDIRECT_V6
        if (appsIds.count() > 0) {
            std::vector<FWPM_FILTER_CONDITION> conditions;
            conditions.reserve(appsIds.count() + 1);

            for (size_t i = 0; i < appsIds.count(); ++i) {
                FWPM_FILTER_CONDITION condition;
                condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
                condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)appsIds.getAppId(i);
                conditions.push_back(condition);
            }

            {
                FWPM_FILTER_CONDITION condition;
                condition.fieldKey = FWPM_CONDITION_IP_PROTOCOL;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_UINT8;
                condition.conditionValue.uint8 = IPPROTO_TCP;
                conditions.push_back(condition);
            }

            FWPM_FILTER filter = { 0 };
            filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
            filter.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
            filter.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W " TCP filter for callout driver (IPv6)";
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x00;
            filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IPV6_GUID;
            filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
            filter.numFilterConditions = static_cast<UINT32>(conditions.size());
            filter.filterCondition = &conditions[0];
            filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
            filter.action.calloutKey = TCP_CALLOUT_V6_GUID;

            UINT64 filterId;
            DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
            retValue = (ret == ERROR_SUCCESS);
            if (!retValue) {
                spdlog::error("CalloutFilter::addFilter(), TCP v6 redirect filter failed: {}", ret);
                return retValue;
            }
        }

        // V6 non-TCP (bind) filter for apps → ALE_BIND_REDIRECT_V6
        if (appsIds.count() > 0) {
            std::vector<FWPM_FILTER_CONDITION> conditions;
            conditions.reserve(appsIds.count() + 1);
            for (size_t i = 0; i < appsIds.count(); ++i) {
                FWPM_FILTER_CONDITION condition;
                condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
                condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)appsIds.getAppId(i);
                conditions.push_back(condition);
            }

            {
                FWPM_FILTER_CONDITION condition;
                condition.fieldKey = FWPM_CONDITION_IP_PROTOCOL;
                condition.matchType = FWP_MATCH_NOT_EQUAL;
                condition.conditionValue.type = FWP_UINT8;
                condition.conditionValue.uint8 = IPPROTO_TCP;
                conditions.push_back(condition);
            }

            FWPM_FILTER filter = { 0 };
            filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
            filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V6;
            filter.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W " bind filter for callout driver (IPv6)";
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x00;
            filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IPV6_GUID;
            filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
            filter.numFilterConditions = static_cast<UINT32>(conditions.size());
            filter.filterCondition = &conditions[0];
            filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
            filter.action.calloutKey = BIND_CALLOUT_V6_GUID;

            UINT64 filterId;
            DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
            retValue = (ret == ERROR_SUCCESS);
            if (!retValue) {
                spdlog::error("CalloutFilter::addFilter(), bind v6 filter failed: {}", ret);
                return retValue;
            }
        }

        // V6 ctrld filter → ALE_CONNECT_REDIRECT_V6
        if (ctrldAppId.count() > 0) {
            std::vector<FWPM_FILTER_CONDITION> conditions;
            conditions.reserve(ctrldAppId.count());

            for (size_t i = 0; i < ctrldAppId.count(); ++i) {
                FWPM_FILTER_CONDITION condition;
                condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
                condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)ctrldAppId.getAppId(i);
                conditions.push_back(condition);
            }

            FWPM_FILTER filter = { 0 };
            filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
            filter.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
            filter.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W " ctrld filter for callout driver (IPv6)";
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x00;
            filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IPV6_GUID;
            filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
            filter.numFilterConditions = static_cast<UINT32>(conditions.size());
            filter.filterCondition = &conditions[0];
            filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
            filter.action.calloutKey = TCP_CALLOUT_V6_GUID;

            UINT64 filterId;
            DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
            retValue = (ret == ERROR_SUCCESS);
            if (!retValue) {
                spdlog::error("CalloutFilter::addFilter(), ctrld v6 redirect filter failed: {}", ret);
                return retValue;
            }
        }
    }

    // Anti-leak v6 on VPN interface for inclusive split tunneling.
    // Closes the leak where non-included apps fall through to a helper-managed bound ::/0
    // on the VPN adapter (any protocol that puts one there: WireGuard, OpenVPN, etc.) when
    // the ISP has no v6 default of its own.
    if (withV6 && !isExclude && vpnIfIndex != 0 && appsIds.count() > 0) {
        NET_LUID vpnLuid = { 0 };
        if (ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(vpnIfIndex), &vpnLuid) == NO_ERROR) {
            // 1) BLOCK all v6 traffic on VPN LUID (weight=4).
            // 2) PERMIT v6 traffic on VPN LUID for appsIds (weight=6, beats the block in arbitration).
            // Non-included apps that fell through to bound ::/0 get blocked here; included apps
            // post callout-redirect to the VPN address are permitted. Filters are bound to the
            // VPN LUID, so traffic on the ISP interface is not affected.
            // persistent=false: SUBLAYER_CALLOUT_GUID itself is transient, and adding a
            // persistent filter to a transient sublayer fails with FWP_E_LIFETIME_MISMATCH.
            bool ok = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 4,
                                         SUBLAYER_CALLOUT_GUID,
                                         (wchar_t *)WS_PRODUCT_NAME_W L" v6 leak block on VPN",
                                         &vpnLuid, nullptr, 0, 0, nullptr, false);
            if (!ok) {
                spdlog::error("CalloutFilter::addFilters(), v6 leak BLOCK on VPN LUID failed");
                return false;
            }

            AppsIds permitAppsIds = appsIds;
            ok = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 6,
                                    SUBLAYER_CALLOUT_GUID,
                                    (wchar_t *)WS_PRODUCT_NAME_W L" v6 leak permit on VPN for apps",
                                    &vpnLuid, nullptr, 0, 0, &permitAppsIds, false);
            if (!ok) {
                spdlog::error("CalloutFilter::addFilters(), v6 leak PERMIT on VPN LUID failed");
                return false;
            }
        } else {
            // Non-fatal: split tunneling itself works without the anti-leak filters; the v6
            // leak for non-included apps simply persists. Failing enable() entirely would be
            // worse, so we log and proceed.
            spdlog::error("CalloutFilter::addFilters(), ConvertInterfaceIndexToLuid failed for vpnIfIndex={}", vpnIfIndex);
        }
    }

    return retValue;
}

bool CalloutFilter::deleteSublayer(HANDLE engineHandle)
{
    bool b1 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_BIND_REDIRECT_V4);
    bool b2 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_CONNECT_REDIRECT_V4);
    bool b3 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_BIND_REDIRECT_V6);
    bool b4 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_CONNECT_REDIRECT_V6);
    // Anti-leak v6 filters live on ALE_AUTH_CONNECT_V6 / ALE_AUTH_RECV_ACCEPT_V6 (added by
    // Utils::addFilterV6). Without explicit cleanup here FwpmSubLayerDeleteByKey0 would fail
    // with FWP_E_IN_USE on the next disable()/enable() cycle.
    bool b5 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_AUTH_CONNECT_V6);
    bool b6 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6);
    DWORD dwRet = FwpmSubLayerDeleteByKey0(engineHandle, &SUBLAYER_CALLOUT_GUID);
    return b1 && b2 && b3 && b4 && b5 && b6 && dwRet == NO_ERROR;
}

bool CalloutFilter::removeAllFilters(FwpmWrapper &fwmpWrapper)
{
    HANDLE hEngine = fwmpWrapper.getHandleAndLock();
    fwmpWrapper.beginTransaction();

    DWORD ret = deleteSublayer(hEngine);

    ret = FwpmCalloutDeleteByKey(hEngine, &BIND_CALLOUT_GUID);
    ret = FwpmCalloutDeleteByKey(hEngine, &TCP_CALLOUT_GUID);
    ret = FwpmCalloutDeleteByKey(hEngine, &BIND_CALLOUT_V6_GUID);
    ret = FwpmCalloutDeleteByKey(hEngine, &TCP_CALLOUT_V6_GUID);
    ret = FwpmProviderContextDeleteByKey(hEngine, &CALLOUT_PROVIDER_CONTEXT_IP_GUID);
    ret = FwpmProviderContextDeleteByKey(hEngine, &CALLOUT_PROVIDER_CONTEXT_IPV6_GUID);

    fwmpWrapper.endTransaction();
    fwmpWrapper.unlock();

    return true;
}
