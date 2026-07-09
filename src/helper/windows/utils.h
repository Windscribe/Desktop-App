#pragma once

#include <windows.h>
#include <ifdef.h>

#include "apps_ids.h"
#include "types/ipaddress.h"

namespace Utils
{
    bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID);
    bool deleteAllFiltersForSublayer(HANDLE engineHandle, const GUID *subLayerGUID, GUID layerKey);

    std::string readAllFromPipe(HANDLE hPipe);
    std::wstring guidToStr(const GUID &guid);
    std::wstring getExePath();
    std::wstring getDirPathFromFullPath(std::wstring &fullPath);
    std::wstring getConfigPath();
    // create=true ensures the directory exists (for staging); cleanup callers should
    // pass false so they don't recreate the dir on the same call that removes it.
    std::wstring getUpdatePath(bool create = true);
    std::wstring getSystemDir();
    bool isValidFileName(std::wstring &filename);
    bool isFileExists(const wchar_t *path);
    // Creates an empty file (replacing any existing one) whose DACL grants access only to SYSTEM and
    // Administrators, for secrets written under Program Files where files otherwise inherit read access for all users.
    bool createRestrictedFile(const std::wstring &path);
    bool hasWhitespaceInString(const std::wstring &str);
    bool verifyAppProcessPath(HANDLE hPipe);
    bool iequals(const std::wstring &a, const std::wstring &b);

    bool setNetworkAdapterState(ULONG ifIndex, bool enable);
    GUID guidFromString(const std::wstring &str);

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

    std::string ssidFromInterfaceGUID(const std::wstring &interfaceGUID);

    void debugOut(const char* format, ...);
    void disableFirewall();
}
