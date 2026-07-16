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

bool CalloutFilter::enable(UINT32 localIp, UINT32 vpnIp,
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
        return true;
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

    // The v6 redirect machinery needs the address the driver actually rewrites the app's
    // source to, and that is mode-dependent (CalloutFunctions.c, ClassifyFnV6): exclusive
    // rewrites to the physical adapter's v6 (localIpV6) and never reads vpnIp; inclusive
    // rewrites to the tunnel's v6. Requiring vpnIpV6 in exclusive mode would withhold a
    // working redirect over the adapter-info race (tunnel v6 not yet assigned at capture)
    // and degrade excluded apps to the per-app v6 BLOCK below — v4-only for the session.
    const bool localV6Ok = localIpV6.isValid() && localIpV6.isV6();
    const bool vpnV6Ok = vpnIpV6.isValid() && vpnIpV6.isV6();
    const bool withV6 = isExclude ? localV6Ok : (localV6Ok && vpnV6Ok);

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

    // Activate the per-destination v6 filters (see setV6WhitelistIps) and reapply any IPs
    // pushed before this enable() ran (HostnamesManager::enable() fires first in
    // SplitTunneling::updateState()). Inclusive gating matches the anti-leak BLOCK in
    // addFilters(); the exclusive case is not gated on appsIds — hostname/CIDR exclusions
    // work without any excluded apps.
    //
    // The exclusive BLOCK is deliberately unconditional (not gated on withV6): it enforces
    // "excluded destinations' v6 never egresses the tunnel" regardless of why off-tunnel
    // steering is unavailable. When IpRoutes *can* steer (physical adapter has both a v6
    // address and a v6 gateway — selected independently in AdapterUtils_win), the traffic
    // uses the physical LUID and the BLOCK never matches; when it can't (no address, no
    // gateway despite a global address — static config or expired RA, host-route races,
    // third-party route interference), the blocked v6 falls back to v4, which the v4 host
    // routes carry outside the tunnel.
    const bool v6InclusiveAntiLeak = !isExclude && appsIds.count() > 0;
    const bool v6ExclusiveDestinationBlock = isExclude;
    if (success && vpnIfIndex != 0 && (v6InclusiveAntiLeak || v6ExclusiveDestinationBlock)) {
        NET_LUID vpnLuid = { 0 };
        if (ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(vpnIfIndex), &vpnLuid) == NO_ERROR) {
            v6AntiLeakActive_ = true;
            v6AntiLeakIsExclude_ = isExclude;
            v6AntiLeakVpnLuid_ = vpnLuid.Value;
            // Part of the fail-loud contract: without these filters the split tunnel
            // either over-blocks hostname-routed destinations (inclusive) or leaks
            // excluded destinations' v6 (exclusive), so a failed install fails enable().
            if (!applyV6WhitelistFiltersLocked(hEngine)) {
                success = false;
            }
        } else {
            // Reachable only in exclusive mode when addFilters() didn't already convert
            // (and fail on) the same index — no per-app v6 BLOCK was installed there.
            // Fail enable() rather than silently leak excluded destinations' v6.
            spdlog::error("CalloutFilter::enable(), ConvertInterfaceIndexToLuid failed for vpnIfIndex={}", vpnIfIndex);
            success = false;
        }
    }

    if (success) {
        fwmpWrapper_.endTransaction();
        isEnabled_ = true;
    } else {
        fwmpWrapper_.abortTransaction();
        // The abort discarded everything, including any per-destination filters recorded
        // by applyV6WhitelistFiltersLocked() above — reset the anti-leak bookkeeping to
        // match, as disable() does. The IP list itself is kept for the next enable().
        v6AntiLeakActive_ = false;
        v6AntiLeakIsExclude_ = false;
        v6AntiLeakVpnLuid_ = 0;
        v6WhitelistFilterIds_.clear();
    }
    fwmpWrapper_.unlock();

    return success;
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
    // it up via applyV6WhitelistFiltersLocked().
    v6AntiLeakActive_ = false;
    v6AntiLeakIsExclude_ = false;
    v6AntiLeakVpnLuid_ = 0;
    v6WhitelistFilterIds_.clear();
}

void CalloutFilter::setV6WhitelistIps(const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    if (v6WhitelistIps_ == ips) {
        return;
    }

    // Not active yet — just hold the list for a future enable().
    if (!v6AntiLeakActive_) {
        v6WhitelistIps_ = ips;
        return;
    }

    // Snapshot for rollback: committing the transaction below with only the removals in
    // it (apply failed) would drop the per-destination filters entirely — in exclusive
    // mode that silently leaks excluded destinations' v6 into the tunnel.
    const std::vector<types::IpAddressRange> prevIps = std::move(v6WhitelistIps_);
    const std::vector<UINT64> prevFilterIds = v6WhitelistFilterIds_;
    v6WhitelistIps_ = ips;

    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    removeV6WhitelistFiltersLocked(hEngine);
    if (applyV6WhitelistFiltersLocked(hEngine)) {
        fwmpWrapper_.endTransaction();
    } else {
        // The abort restores the just-deleted filters in WFP; restore the bookkeeping to
        // match (remove cleared the id list, apply may have recorded partial ids that no
        // longer exist). Restoring the old IP list also keeps the equality early-return
        // above from suppressing a retry when the same resolution arrives again.
        fwmpWrapper_.abortTransaction();
        v6WhitelistIps_ = prevIps;
        v6WhitelistFilterIds_ = prevFilterIds;
    }
    fwmpWrapper_.unlock();
}

bool CalloutFilter::applyV6WhitelistFiltersLocked(HANDLE engineHandle)
{
    if (!v6AntiLeakActive_ || v6WhitelistIps_.empty()) {
        return true;
    }

    // addFilterV6 internally skips ranges whose family doesn't match and turns into a
    // no-op when no v6 entries remain, so passing the dual-stack vector unfiltered is
    // safe. It does, however, pack all matching ranges into a single filter via repeated
    // FWPM_CONDITION_IP_REMOTE_ADDRESS conditions (WFP OR's same-field conditions),
    // which keeps the per-packet cost flat as the list grows.
    //
    // persistent=false in both branches: SUBLAYER_CALLOUT_GUID is transient; persistent
    // filters in a transient sublayer fail to register with FWP_E_LIFETIME_MISMATCH.
    NET_LUID vpnLuid;
    vpnLuid.Value = v6AntiLeakVpnLuid_;

    bool ok;
    if (v6AntiLeakIsExclude_) {
        // Exclusive: BLOCK excluded destinations' v6 on the VPN LUID. Weight 4 for
        // consistency with the other anti-leak BLOCKs; nothing competes with it here.
        ok = Utils::addFilterV6(engineHandle, &v6WhitelistFilterIds_, FWP_ACTION_BLOCK, 4,
                                SUBLAYER_CALLOUT_GUID,
                                (wchar_t *)WS_PRODUCT_NAME_W L" v6 excluded destinations block on VPN",
                                &vpnLuid, &v6WhitelistIps_, 0, 0, nullptr, false);
    } else {
        // Inclusive: PERMIT through the anti-leak BLOCK. Weight 5 — above the BLOCK (4);
        // relative order against the per-app PERMIT (6) doesn't matter between two PERMITs.
        ok = Utils::addFilterV6(engineHandle, &v6WhitelistFilterIds_, FWP_ACTION_PERMIT, 5,
                                SUBLAYER_CALLOUT_GUID,
                                (wchar_t *)WS_PRODUCT_NAME_W L" v6 whitelist permit on VPN",
                                &vpnLuid, &v6WhitelistIps_, 0, 0, nullptr, false);
    }
    if (!ok) {
        spdlog::error("CalloutFilter::applyV6WhitelistFiltersLocked(), addFilterV6 failed (isExclude={})", v6AntiLeakIsExclude_);
    }
    return ok;
}

void CalloutFilter::removeV6WhitelistFiltersLocked(HANDLE engineHandle)
{
    for (UINT64 id : v6WhitelistFilterIds_) {
        DWORD ret = FwpmFilterDeleteById0(engineHandle, id);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("CalloutFilter::removeV6WhitelistFiltersLocked(), FwpmFilterDeleteById0 failed: {}", ret);
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
    //
    // Deliberately NOT gated on withV6: the BLOCK is needed most when the physical adapter
    // has no v6 address at all (localIpV6 invalid — a dual-stack tunnel is then the only v6
    // path in the system), and it is safe to install before the tunnel's v6 address is
    // assigned (vpnIpV6 invalid — the adapter-info race). withV6 only gates the redirect
    // machinery, which needs the mode's redirect target (see the withV6 definition in
    // enable()).
    if (!isExclude && vpnIfIndex != 0 && appsIds.count() > 0) {
        NET_LUID vpnLuid = { 0 };
        if (ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(vpnIfIndex), &vpnLuid) != NO_ERROR) {
            // This is now the only path that would silently skip the anti-leak BLOCK, so
            // treat it as fatal: fail addFilters() so enable() does not report a working
            // split tunnel that leaks v6.
            spdlog::error("CalloutFilter::addFilters(), ConvertInterfaceIndexToLuid failed for vpnIfIndex={}", vpnIfIndex);
            return false;
        }

        // 1) BLOCK all v6 traffic on VPN LUID (weight=4).
        // 2) PERMIT v6 traffic on VPN LUID for appsIds + ctrldAppId (weight=6, beats the block in arbitration).
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

        // ctrldAppId (main app + ctrld) isn't part of appsIds but is redirected into the
        // tunnel too — its v6 must not be vetoed by the BLOCK above.
        AppsIds permitAppsIds = appsIds;
        permitAppsIds.addFrom(ctrldAppId);
        ok = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 6,
                                SUBLAYER_CALLOUT_GUID,
                                (wchar_t *)WS_PRODUCT_NAME_W L" v6 leak permit on VPN for apps",
                                &vpnLuid, nullptr, 0, 0, &permitAppsIds, false);
        if (!ok) {
            spdlog::error("CalloutFilter::addFilters(), v6 leak PERMIT on VPN LUID failed");
            return false;
        }
    }

    // Mirror case for exclusive split tunneling: without a valid localIpV6 there is no
    // address to redirect excluded apps' v6 traffic to, so the v6 redirect filters are not
    // installed (withV6 == false) and excluded apps' v6 traffic follows the default route
    // into the tunnel — an excluded app would show the VPN v6 IP. Block v6 on the VPN LUID
    // for the excluded apps only; they fall back to v4, which the callout correctly steers
    // outside the tunnel. Other apps are unaffected: in exclusive mode they are supposed to
    // use the tunnel, including over v6 (excluded hostname/CIDR destinations are handled
    // separately via setV6WhitelistIps).
    if (isExclude && !withV6 && vpnIfIndex != 0 && appsIds.count() > 0) {
        NET_LUID vpnLuid = { 0 };
        if (ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(vpnIfIndex), &vpnLuid) != NO_ERROR) {
            // Same reasoning as the inclusive branch above: this is the only way to silently
            // end up without the block, so fail loudly.
            spdlog::error("CalloutFilter::addFilters(), ConvertInterfaceIndexToLuid failed for vpnIfIndex={}", vpnIfIndex);
            return false;
        }

        // persistent=false for the same FWP_E_LIFETIME_MISMATCH reason as above. Weight 4
        // for consistency with the inclusive BLOCK; nothing competes with it in this
        // sublayer in exclusive mode.
        AppsIds blockAppsIds = appsIds;
        bool ok = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 4,
                                     SUBLAYER_CALLOUT_GUID,
                                     (wchar_t *)WS_PRODUCT_NAME_W L" v6 leak block on VPN for excluded apps",
                                     &vpnLuid, nullptr, 0, 0, &blockAppsIds, false);
        if (!ok) {
            spdlog::error("CalloutFilter::addFilters(), v6 leak BLOCK for excluded apps on VPN LUID failed");
            return false;
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
