#pragma once

#include <winsock2.h>
#include <windows.h>
#include <fwpmu.h>
#include <ifdef.h>

#include <cstdint>
#include <vector>

#include "apps_ids.h"
#include "types/ipaddress.h"

// Windows Filtering Platform helpers shared by the firewall sublayers (FirewallFilter, DnsFirewall,
// Ipv6Firewall, CalloutFilter). Kept apart from utils.h so they can be built into tests without the
// rest of the helper.
namespace Utils
{
    // Persistent WFP provider that tags every filter the helper adds. Enumerating with it as the
    // template's providerKey makes BFE return only our filters, instead of every filter on the layer
    // (about 1,250 on the four ALE auth layers of a typical machine), which cuts deleting a
    // sublayer's filters from ~75 ms to 1-12 ms. Not branded, like the sublayer GUIDs.
    // {0836a32c-3afd-44e5-a5f4-7ae4d3fc0a8e}
    inline constexpr GUID kWfpProviderGuid = { 0x0836a32c, 0x3afd, 0x44e5, { 0xa5, 0xf4, 0x7a, 0xe4, 0xd3, 0xfc, 0x0a, 0x8e } };

    // Adds the provider if it is missing. Filters are tagged only while it is known to exist, so a
    // failure here costs speed, not protection. Called from FwpmWrapper::initialize().
    bool ensureWfpProvider(HANDLE engineHandle);
    // Deletes the provider (uninstall). Call only after every tagged filter is gone.
    void deleteWfpProvider(HANDLE engineHandle);

    // FwpmFilterAdd0 with filter->providerKey set to our provider when it is available. If the
    // provider has disappeared since (FWP_E_PROVIDER_NOT_FOUND), the filter is added untagged rather
    // than not at all: the firewall must never fail open over a missing tag.
    DWORD addFilterWithProvider(HANDLE engineHandle, FWPM_FILTER0 *filter, UINT64 *id);

    // Deletes every filter in the sublayer on the given layers, then the sublayer itself. Returns
    // true if the sublayer is gone (or never existed). Filters are found through our provider; if the
    // sublayer is still in use afterwards (untagged filters, e.g. persisted by an older version), it
    // falls back to scanning the whole layers. The overload without layers covers the four ALE auth
    // layers that addFilterV4/addFilterV6 use.
    bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID);
    bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID, const std::vector<GUID> &layers);

    // Both addFilterV4/addFilterV6 accept dual-stack types::IpAddressRange ranges; entries that
    // don't match the filter's family are skipped with a debug log. Callers may pass mixed
    // containers without pre-splitting (the common case being already-typed input).
    bool addFilterV4(HANDLE engineHandle, std::vector<UINT64> *filterId, FWP_ACTION_TYPE type, UINT8 weight,
                     GUID subLayerKey, wchar_t *subLayerName, PNET_LUID pluid = nullptr,
                     const std::vector<types::IpAddressRange> *range = nullptr,
                     uint16_t localPort = 0, uint16_t remotePort = 0, AppsIds *appsIds = nullptr, bool persistent = true);
    bool addFilterV6(HANDLE engineHandle, std::vector<UINT64> *filterId, FWP_ACTION_TYPE type, UINT8 weight,
                     GUID subLayerKey, wchar_t *subLayerName, PNET_LUID pluid = nullptr,
                     const std::vector<types::IpAddressRange> *range = nullptr,
                     uint16_t localPort = 0, uint16_t remotePort = 0, AppsIds *appsIds = nullptr, bool persistent = true);

    // Permit ICMPv6 messages whose type is in [typeLow, typeHigh] (inclusive) at both ALE v6 layers.
    // Used for NDP/MLD (types 130-137 and 143), which must be allowed regardless of the Allow LAN
    // setting. FWPM_CONDITION_ICMP_TYPE aliases IP_LOCAL_PORT at the ALE layers, so it is paired with
    // an IP_PROTOCOL == IPPROTO_ICMPV6 condition to avoid matching non-ICMP traffic on the same field.
    bool addPermitFilterV6IcmpTypeRange(HANDLE engineHandle, uint16_t typeLow, uint16_t typeHigh, UINT8 weight,
                     GUID subLayerKey, wchar_t *subLayerName, bool persistent = true);
}
