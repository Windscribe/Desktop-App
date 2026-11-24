#include "ws_branding.h"
#include "wfp_utils.h"

#include <Fwpmu.h>
#include <ws2def.h>

#include <atomic>

#include <spdlog/spdlog.h>

#include "utils/log/debug_trace.h"

namespace Utils
{

namespace
{

// True once ensureWfpProvider has seen the provider exist; cleared if a filter add finds it gone.
std::atomic<bool> g_providerAvailable{false};

const GUID *providerKeyIfAvailable()
{
    return g_providerAvailable.load() ? &kWfpProviderGuid : nullptr;
}

// The layers addFilterV4/addFilterV6/addPermitFilterV6IcmpTypeRange add filters to.
const std::vector<GUID> &aleAuthLayers()
{
    static const std::vector<GUID> layers = {
        FWPM_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
        FWPM_LAYER_ALE_AUTH_CONNECT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 };
    return layers;
}

// Deletes the sublayer's filters on one layer and adds the number deleted to `deleted`. With a
// providerKey, BFE returns only that provider's filters; with nullptr it returns every filter on the
// layer. Either way the sublayer key decides what is deleted.
bool deleteFiltersOnLayer(HANDLE engineHandle, const GUID &subLayerGUID, const GUID &layerKey,
                          const GUID *providerKey, UINT32 &deleted)
{
    FWPM_FILTER_ENUM_TEMPLATE0 enumTemplate = {};
    enumTemplate.layerKey = layerKey;
    enumTemplate.actionMask = 0xFFFFFFFF;
    enumTemplate.providerKey = const_cast<GUID *>(providerKey);
    HANDLE enumHandle = nullptr;
    DWORD dwRet = FwpmFilterCreateEnumHandle0(engineHandle, &enumTemplate, &enumHandle);
    if (dwRet != ERROR_SUCCESS) {
        spdlog::error("deleteFiltersOnLayer - FwpmFilterCreateEnumHandle0 failed: {}", dwRet);
        return false;
    }

    bool success = true;
    FWPM_FILTER0 **filters = nullptr;
    UINT32 numFiltersReturned = 0;
    dwRet = FwpmFilterEnum0(engineHandle, enumHandle, INFINITE, &filters, &numFiltersReturned);
    while (dwRet == ERROR_SUCCESS && numFiltersReturned > 0) {
        for (UINT32 i = 0; i < numFiltersReturned; ++i) {
            if (filters[i]->subLayerKey == subLayerGUID) {
                const DWORD deleteRet = FwpmFilterDeleteById0(engineHandle, filters[i]->filterId);
                if (deleteRet == ERROR_SUCCESS) {
                    ++deleted;
                } else {
                    spdlog::error("deleteFiltersOnLayer - FwpmFilterDeleteById0 failed for filter {}: {}", filters[i]->filterId, deleteRet);
                    success = false;
                }
            }
        }

        FwpmFreeMemory0((void **)&filters);
        dwRet = FwpmFilterEnum0(engineHandle, enumHandle, INFINITE, &filters, &numFiltersReturned);
    }
    if (dwRet != ERROR_SUCCESS) {
        spdlog::error("deleteFiltersOnLayer - FwpmFilterEnum0 failed: {}", dwRet);
        success = false;
    }
    FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
    return success;
}

} // namespace

bool ensureWfpProvider(HANDLE engineHandle)
{
    FWPM_PROVIDER0 provider = {};
    provider.providerKey = kWfpProviderGuid;
    provider.displayData.name = (wchar_t *)WS_PRODUCT_NAME_W;
    provider.displayData.description = (wchar_t *)WS_PRODUCT_NAME_W L" firewall";
    // Persistent, because the persistent filters (the firewall-on-boot set) may only reference
    // persistent objects.
    provider.flags = FWPM_PROVIDER_FLAG_PERSISTENT;

    const DWORD ret = FwpmProviderAdd0(engineHandle, &provider, NULL);
    const bool available = (ret == ERROR_SUCCESS || ret == FWP_E_ALREADY_EXISTS);
    if (!available) {
        spdlog::error("ensureWfpProvider - FwpmProviderAdd0 failed: {}; filters will be added untagged", ret);
    }
    g_providerAvailable.store(available);
    return available;
}

void deleteWfpProvider(HANDLE engineHandle)
{
    g_providerAvailable.store(false);
    const DWORD ret = FwpmProviderDeleteByKey0(engineHandle, &kWfpProviderGuid);
    if (ret != ERROR_SUCCESS && ret != FWP_E_PROVIDER_NOT_FOUND) {
        spdlog::error("deleteWfpProvider - FwpmProviderDeleteByKey0 failed: {}", ret);
    }
}

DWORD addFilterWithProvider(HANDLE engineHandle, FWPM_FILTER0 *filter, UINT64 *id)
{
    const GUID *providerKey = providerKeyIfAvailable();
    filter->providerKey = const_cast<GUID *>(providerKey);
    DWORD ret = FwpmFilterAdd0(engineHandle, filter, NULL, id);
    if (ret == FWP_E_PROVIDER_NOT_FOUND && providerKey != nullptr) {
        // Someone deleted the provider after we checked for it. An untagged filter still protects;
        // deleting it later just takes the slower full-scan path.
        spdlog::error("addFilterWithProvider - the WFP provider is gone, adding filters untagged");
        g_providerAvailable.store(false);
        filter->providerKey = nullptr;
        ret = FwpmFilterAdd0(engineHandle, filter, NULL, id);
    }
    return ret;
}

bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID)
{
    return deleteSublayerAndAllFilters(engineHandle, subLayerGUID, aleAuthLayers());
}

bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID, const std::vector<GUID> &layers)
{
    WS_DEBUG_TRACE_SCOPE("Utils::deleteSublayerAndAllFilters");

    // WFP rejects a filter whose sublayer does not exist, so an absent sublayer has no filters left.
    FWPM_SUBLAYER0 *subLayer = nullptr;
    DWORD result = FwpmSubLayerGetByKey0(engineHandle, subLayerGUID, &subLayer);
    if (result == FWP_E_SUBLAYER_NOT_FOUND) {
        return true;
    }
    if (result == ERROR_SUCCESS) {
        FwpmFreeMemory0((void **)&subLayer);
    } else {
        // Go on with the cleanup rather than risk leaving filters behind.
        spdlog::error("deleteSublayerAndAllFilters - FwpmSubLayerGetByKey0 failed: {}", result);
    }

    UINT32 deleted = 0;
    const GUID *providerKey = providerKeyIfAvailable();
    if (providerKey != nullptr) {
        for (const GUID &layer : layers) {
            deleteFiltersOnLayer(engineHandle, *subLayerGUID, layer, providerKey, deleted);
        }
        result = FwpmSubLayerDeleteByKey0(engineHandle, subLayerGUID);
        if (result != FWP_E_IN_USE) {
            if (result != ERROR_SUCCESS && result != FWP_E_SUBLAYER_NOT_FOUND) {
                spdlog::error("deleteSublayerAndAllFilters - FwpmSubLayerDeleteByKey0 failed: {}", result);
            }
            spdlog::debug("deleteSublayerAndAllFilters - deleted {} filters", deleted);
            return result == ERROR_SUCCESS || result == FWP_E_SUBLAYER_NOT_FOUND;
        }
        // WFP will not delete a sublayer that filters still reference, so untagged filters remain:
        // ones persisted by a version before the provider existed, or added while it was missing.
        spdlog::warn("deleteSublayerAndAllFilters - deleted {} tagged filters, but the sublayer is still in use; scanning all filters", deleted);
    }

    for (const GUID &layer : layers) {
        deleteFiltersOnLayer(engineHandle, *subLayerGUID, layer, nullptr, deleted);
    }
    result = FwpmSubLayerDeleteByKey0(engineHandle, subLayerGUID);
    if (result != ERROR_SUCCESS && result != FWP_E_SUBLAYER_NOT_FOUND) {
        spdlog::error("deleteSublayerAndAllFilters - FwpmSubLayerDeleteByKey0 failed: {}", result);
    }
    spdlog::debug("deleteSublayerAndAllFilters - deleted {} filters (full scan)", deleted);
    return result == ERROR_SUCCESS || result == FWP_E_SUBLAYER_NOT_FOUND;
}

bool addFilterV4(HANDLE engineHandle, std::vector<UINT64> *filterId, FWP_ACTION_TYPE type, UINT8 weight,
                 GUID subLayerKey, wchar_t *subLayerName, PNET_LUID pluid,
                 const std::vector<types::IpAddressRange> *ranges,
                 uint16_t localPort, uint16_t remotePort, AppsIds *appsIds, bool persistent)
{
    UINT64 id = 0;
    bool success = true;
    DWORD dwFwApiRetCode = 0;
    // FWPM v4 filter conditions take FWP_V4_ADDR_AND_MASK by pointer; we have to keep the storage
    // alive until FwpmFilterAdd0 returns. Reserve up to one slot per range; non-V4 entries are
    // skipped (they have no V4 mask), so we don't pre-size to ranges->size() exactly.
    std::vector<FWP_V4_ADDR_AND_MASK> addrMasks;
    if (ranges) {
        addrMasks.reserve(ranges->size());
    }
    // Dual-stack guard: if a caller passes a non-empty range vector containing only non-v4
    // entries, treat it as "no v4 ranges to match" and skip filter creation. Without this guard
    // we'd register a filter with no IP condition, i.e. permit/block ALL v4 traffic — clearly
    // not what the caller intended.
    if (ranges && !ranges->empty()) {
        bool anyV4 = false;
        for (const auto &r : *ranges) {
            if (r.isV4()) { anyV4 = true; break; }
        }
        if (!anyV4) {
            return true;
        }
    }
    GUID guids[2] = {FWPM_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4};

    for (int i = 0; i < 2; i++) {
        FWPM_FILTER0 filter = { 0 };
        std::vector<FWPM_FILTER_CONDITION0> conditions;

        filter.subLayerKey = subLayerKey;
        filter.displayData.name = subLayerName;
        filter.layerKey = guids[i];
        filter.action.type = type;
        filter.flags = 0;
        if (persistent) {
            filter.flags |= FWPM_FILTER_FLAG_PERSISTENT;
        }
        // only veto block rules
        if (type == FWP_ACTION_BLOCK) {
            filter.flags |= FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;
        }
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = weight;

        if (ranges) {
            // Reset masks for the second pass so we don't keep both layers' entries.
            addrMasks.clear();
            for (const auto &r : *ranges) {
                if (!r.isV4()) {
                    continue;
                }
                FWP_V4_ADDR_AND_MASK m;
                m.addr = r.address().ipv4HostOrder();
                m.mask = r.ipv4MaskHostOrder();
                addrMasks.push_back(m);

                FWPM_FILTER_CONDITION0 condition;
                condition.fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_V4_ADDR_MASK;
                condition.conditionValue.v4AddrMask = &addrMasks.back();
                conditions.push_back(condition);
            }
        }

        if (pluid != nullptr) {
            FWPM_FILTER_CONDITION0 condition;
            condition.fieldKey = FWPM_CONDITION_IP_LOCAL_INTERFACE;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT64;
            condition.conditionValue.uint64 = &pluid->Value;
            conditions.push_back(condition);
        }

        if (localPort != 0) {
            FWPM_FILTER_CONDITION0 condition;
            condition.fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT16;
            condition.conditionValue.uint16 = localPort;
            conditions.push_back(condition);
        }

        if (remotePort != 0) {
            FWPM_FILTER_CONDITION0 condition;
            condition.fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT16;
            condition.conditionValue.uint16 = remotePort;
            conditions.push_back(condition);
        }

        if (appsIds != nullptr) {
            for (size_t i = 0; i < (*appsIds).count(); ++i) {
                FWPM_FILTER_CONDITION0 condition;
                condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
                condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)(*appsIds).getAppId(i);
                conditions.push_back(condition);
            }
        }

        filter.filterCondition = conditions.data();
        filter.numFilterConditions = conditions.size();

        dwFwApiRetCode = addFilterWithProvider(engineHandle, &filter, &id);
        if (dwFwApiRetCode != ERROR_SUCCESS) {
            spdlog::error("Error adding filter: {}", dwFwApiRetCode);
            success = false;
        } else if (filterId) {
            // Only record the id on success: on failure FwpmFilterAdd0 leaves it undefined and
            // a stale value would be picked up later by cleanup paths trying to delete filters.
            (*filterId).push_back(id);
        }
    }
    return success;
}

bool addFilterV6(HANDLE engineHandle, std::vector<UINT64> *filterId, FWP_ACTION_TYPE type, UINT8 weight,
                 GUID subLayerKey, wchar_t *subLayerName, PNET_LUID pluid,
                 const std::vector<types::IpAddressRange> *ranges,
                 uint16_t localPort, uint16_t remotePort, AppsIds *appsIds, bool persistent)
{
    UINT64 id = 0;
    bool success = true;
    DWORD dwFwApiRetCode = 0;
    std::vector<FWP_V6_ADDR_AND_MASK> addrMasks;
    if (ranges) {
        addrMasks.reserve(ranges->size());
    }
    // See addFilterV4 for rationale: if ranges are present but contain no v6 entries,
    // skip filter creation rather than registering an "all v6 traffic" rule.
    if (ranges && !ranges->empty()) {
        bool anyV6 = false;
        for (const auto &r : *ranges) {
            if (r.isV6()) { anyV6 = true; break; }
        }
        if (!anyV6) {
            return true;
        }
    }
    GUID guids[2] = {FWPM_LAYER_ALE_AUTH_CONNECT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};

    for (int i = 0; i < 2; i++) {
        FWPM_FILTER0 filter = { 0 };
        std::vector<FWPM_FILTER_CONDITION0> conditions;

        filter.subLayerKey = subLayerKey;
        filter.displayData.name = subLayerName;
        filter.layerKey = guids[i];
        filter.action.type = type;
        filter.flags = 0;
        if (persistent) {
            filter.flags |= FWPM_FILTER_FLAG_PERSISTENT;
        }
        // only veto block rules
        if (type == FWP_ACTION_BLOCK) {
            filter.flags |= FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;
        }
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = weight;

        if (ranges) {
            addrMasks.clear();
            for (const auto &r : *ranges) {
                if (!r.isV6()) {
                    continue;
                }
                FWP_V6_ADDR_AND_MASK m;
                CopyMemory(m.addr, r.address().bytes(), 16);
                m.prefixLength = r.prefixLength();
                addrMasks.push_back(m);

                FWPM_FILTER_CONDITION0 condition;
                condition.fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_V6_ADDR_MASK;
                condition.conditionValue.v6AddrMask = &addrMasks.back();
                conditions.push_back(condition);
            }
        }

        if (pluid != nullptr) {
            FWPM_FILTER_CONDITION0 condition;
            condition.fieldKey = FWPM_CONDITION_IP_LOCAL_INTERFACE;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT64;
            condition.conditionValue.uint64 = &pluid->Value;
            conditions.push_back(condition);
        }

        if (localPort != 0) {
            FWPM_FILTER_CONDITION0 condition;
            condition.fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT16;
            condition.conditionValue.uint16 = localPort;
            conditions.push_back(condition);
        }

        if (remotePort != 0) {
            FWPM_FILTER_CONDITION0 condition;
            condition.fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
            condition.matchType = FWP_MATCH_EQUAL;
            condition.conditionValue.type = FWP_UINT16;
            condition.conditionValue.uint16 = remotePort;
            conditions.push_back(condition);
        }

        if (appsIds != nullptr) {
            for (size_t i = 0; i < (*appsIds).count(); ++i) {
                FWPM_FILTER_CONDITION0 condition;
                condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
                condition.matchType = FWP_MATCH_EQUAL;
                condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
                condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)(*appsIds).getAppId(i);
                conditions.push_back(condition);
            }
        }

        filter.filterCondition = conditions.data();
        filter.numFilterConditions = conditions.size();

        dwFwApiRetCode = addFilterWithProvider(engineHandle, &filter, &id);
        if (dwFwApiRetCode != ERROR_SUCCESS) {
            spdlog::error("Error adding filter: {}", dwFwApiRetCode);
            success = false;
        } else if (filterId) {
            // Only record the id on success: on failure FwpmFilterAdd0 leaves it undefined and
            // a stale value would be picked up later by cleanup paths trying to delete filters.
            (*filterId).push_back(id);
        }
    }
    return success;
}

bool addPermitFilterV6IcmpTypeRange(HANDLE engineHandle, uint16_t typeLow, uint16_t typeHigh, UINT8 weight,
                 GUID subLayerKey, wchar_t *subLayerName, bool persistent)
{
    bool success = true;
    GUID guids[2] = {FWPM_LAYER_ALE_AUTH_CONNECT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};

    // The range storage must outlive the FwpmFilterAdd0 calls below, so it is declared here, ahead
    // of the per-layer loop, and is identical for both layers.
    FWP_VALUE0 rangeLow = {};
    rangeLow.type = FWP_UINT16;
    rangeLow.uint16 = typeLow;
    FWP_VALUE0 rangeHigh = {};
    rangeHigh.type = FWP_UINT16;
    rangeHigh.uint16 = typeHigh;
    FWP_RANGE0 typeRange = {};
    typeRange.valueLow = rangeLow;
    typeRange.valueHigh = rangeHigh;

    for (int i = 0; i < 2; i++) {
        FWPM_FILTER0 filter = { 0 };
        std::vector<FWPM_FILTER_CONDITION0> conditions;

        filter.subLayerKey = subLayerKey;
        filter.displayData.name = subLayerName;
        filter.layerKey = guids[i];
        filter.action.type = FWP_ACTION_PERMIT;
        filter.flags = 0;
        if (persistent) {
            filter.flags |= FWPM_FILTER_FLAG_PERSISTENT;
        }
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = weight;

        FWPM_FILTER_CONDITION0 protoCondition;
        protoCondition.fieldKey = FWPM_CONDITION_IP_PROTOCOL;
        protoCondition.matchType = FWP_MATCH_EQUAL;
        protoCondition.conditionValue.type = FWP_UINT8;
        protoCondition.conditionValue.uint8 = IPPROTO_ICMPV6;
        conditions.push_back(protoCondition);

        FWPM_FILTER_CONDITION0 typeCondition;
        typeCondition.fieldKey = FWPM_CONDITION_ICMP_TYPE;
        typeCondition.matchType = FWP_MATCH_RANGE;
        typeCondition.conditionValue.type = FWP_RANGE_TYPE;
        typeCondition.conditionValue.rangeValue = &typeRange;
        conditions.push_back(typeCondition);

        filter.filterCondition = conditions.data();
        filter.numFilterConditions = conditions.size();

        UINT64 id = 0;
        DWORD dwFwApiRetCode = addFilterWithProvider(engineHandle, &filter, &id);
        if (dwFwApiRetCode != ERROR_SUCCESS) {
            spdlog::error("Error adding ICMPv6 type filter: {}", dwFwApiRetCode);
            success = false;
        }
    }
    return success;
}

} // namespace Utils
