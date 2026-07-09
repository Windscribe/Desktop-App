#include "ws_branding.h"
#include "utils.h"

#include <Fwpmu.h>
#include <ws2def.h>
#include <Psapi.h>
#include <sddl.h>
#include <shlobj.h>
#include <wlanapi.h>
#include <SetupAPI.h>
#include <devguid.h>

#include <filesystem>
#include <sstream>

#include <spdlog/spdlog.h>

#include "dns_firewall.h"
#include "firewallfilter.h"
#include "fwpm_wrapper.h"
#include "ipv6_firewall.h"
#include "split_tunneling/split_tunneling.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif
#include "utils/win32handle.h"
#include "utils/wsscopeguard.h"

namespace Utils
{

bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID)
{
    FWPM_SUBLAYER0 *subLayer = nullptr;
    auto freeSublayer = wsl::wsScopeGuard([&] {
        if (subLayer != nullptr) {
            FwpmFreeMemory0((void**)&subLayer);
        }
    });

    DWORD result = FwpmSubLayerGetByKey0(engineHandle, subLayerGUID, &subLayer);
    if (result == FWP_E_SUBLAYER_NOT_FOUND) {
        return true;
    }

    if (result != ERROR_SUCCESS) {
        spdlog::error("deleteSublayerAndAllFilters failed: {}", result);
        return false;
    }

    deleteAllFiltersForSublayer(engineHandle, subLayerGUID, FWPM_LAYER_ALE_AUTH_CONNECT_V4);
    deleteAllFiltersForSublayer(engineHandle, subLayerGUID, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4);
    deleteAllFiltersForSublayer(engineHandle, subLayerGUID, FWPM_LAYER_ALE_AUTH_CONNECT_V6);
    deleteAllFiltersForSublayer(engineHandle, subLayerGUID, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6);

    FwpmSubLayerDeleteByKey0(engineHandle, subLayerGUID);

    return true;
}

bool deleteAllFiltersForSublayer(HANDLE engineHandle, const GUID *subLayerGUID, GUID layerKey)
{
    FWPM_FILTER_ENUM_TEMPLATE0 enumTemplate;
    ZeroMemory(&enumTemplate, sizeof(enumTemplate));
    enumTemplate.layerKey = layerKey;
    enumTemplate.actionMask = 0xFFFFFFFF;
    HANDLE enumHandle;
    DWORD dwRet = FwpmFilterCreateEnumHandle0(engineHandle, &enumTemplate, &enumHandle);
    if (dwRet != ERROR_SUCCESS) {
        return false;
    }

    FWPM_FILTER0 **filters;
    UINT32 numFiltersReturned;
    dwRet = FwpmFilterEnum0(engineHandle, enumHandle, INFINITE, &filters, &numFiltersReturned);
    while (dwRet == ERROR_SUCCESS && numFiltersReturned > 0) {
        for (UINT32 i = 0; i < numFiltersReturned; ++i) {
            if (filters[i]->subLayerKey == *subLayerGUID) {
                FwpmFilterDeleteById0(engineHandle, filters[i]->filterId);
            }
        }

        FwpmFreeMemory0((void **)&filters);
        dwRet = FwpmFilterEnum0(engineHandle, enumHandle, INFINITE, &filters, &numFiltersReturned);
    }
    dwRet = FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
    return true;
}

std::string readAllFromPipe(HANDLE hPipe)
{
    const int BUFFER_SIZE = 1024;
    std::string csoutput;
    while (true)
    {
        char buf[BUFFER_SIZE + 1];
        DWORD readword;
        if (!::ReadFile(hPipe, buf, BUFFER_SIZE, &readword, 0))
        {
            break;
        }
        if (readword == 0)
        {
            break;
        }
        buf[readword] = '\0';
        std::string cstemp = buf;
        csoutput += cstemp;
    }
    return csoutput;
}

std::wstring guidToStr(const GUID &guid)
{
    wchar_t szBuf[256];
    swprintf(szBuf, 256, L"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return szBuf;
}

std::wstring getExePath()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}

std::wstring getDirPathFromFullPath(std::wstring &fullPath)
{
    size_t found = fullPath.find_last_of(L"/\\");
    if (found != std::wstring::npos)
    {
        return fullPath.substr(0, found);
    }
    else
    {
        return fullPath;
    }
}

bool isValidFileName(std::wstring &filename)
{
    return filename.find_first_of(L"\\/<>|\":?* ") == std::wstring::npos;
}

bool isFileExists(const wchar_t *path)
{
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool createRestrictedFile(const std::wstring &path)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(L"D:P(A;;FA;;;SY)(A;;FA;;;BA)", SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL)) {
        spdlog::error("createRestrictedFile - failed to build security descriptor: {}", ::GetLastError());
        return false;
    }
    auto freeSD = wsl::wsScopeGuard([&] {
        ::LocalFree(sa.lpSecurityDescriptor);
    });

    // Remove any stale file first; CREATE_NEW then guarantees the DACL is applied to a file we
    // created, rather than silently retained from an existing file's descriptor.
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        spdlog::error("createRestrictedFile - could not remove stale file: {}", ec.message());
        return false;
    }

    wsl::Win32Handle hFile(::CreateFileW(path.c_str(), GENERIC_WRITE, 0, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL));
    if (!hFile.isValid()) {
        spdlog::error("createRestrictedFile - could not create file: {}", ::GetLastError());
        return false;
    }
    return true;
}

bool hasWhitespaceInString(const std::wstring &str)
{
    return str.find_first_of(L" \n\r\t") != std::wstring::npos;
}

bool iequals(const std::wstring &a, const std::wstring &b)
{
    if (b.size() != a.size())
        return false;

    return _wcsnicmp(a.c_str(), b.c_str(), a.size()) == 0;
}

bool verifyAppProcessPath(HANDLE hPipe)
{
    DWORD dwStart = ::GetTickCount();

    DWORD pidClient = 0;
    if (!::GetNamedPipeClientProcessId(hPipe, &pidClient)) {
        spdlog::error("verifyAppProcessPath GetNamedPipeClientProcessId failed {}", ::GetLastError());
        return false;
    }

    wsl::Win32Handle processHandle(::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pidClient));
    if (!processHandle.isValid()) {
        spdlog::error("verifyAppProcessPath OpenProcess failed {}", ::GetLastError());
        return false;
    }

    // Reject Session 0 callers. Post-Vista, services and other non-interactive contexts live in
    // Session 0 and have no desktop, so the real user-facing app is never there — any client that
    // claims to be is either misconfigured or hostile.
    DWORD clientSessionId = 0;
    if (!::ProcessIdToSessionId(pidClient, &clientSessionId)) {
        spdlog::error("verifyAppProcessPath ProcessIdToSessionId failed {}", ::GetLastError());
        return false;
    }
    if (clientSessionId == 0) {
        spdlog::error("verifyAppProcessPath rejecting client pid {} in Session 0", pidClient);
        return false;
    }

    wchar_t path[MAX_PATH];
    if (::GetModuleFileNameEx(processHandle.getHandle(), NULL, path, MAX_PATH) == 0) {
        spdlog::error("verifyAppProcessPath GetModuleFileNameEx failed {}", ::GetLastError());
        return false;
    }

    std::wstring appExePath = getExePath() + std::wstring(L"\\" WS_PRODUCT_NAME_W L".exe");

    if (!iequals(appExePath, path)) {
        spdlog::error(L"verifyAppProcessPath invalid process path: {}", path);
        return false;
    }

#if defined(USE_SIGNATURE_CHECK)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(path)) {
        spdlog::error("verifyAppProcessPath signature verify failed. Err = {}", sigCheck.lastError());
        return false;
    }
#endif

    DWORD dwElapsed = ::GetTickCount() - dwStart;
    spdlog::debug(L"verifyAppProcessPath signature verified for {} in {} ms", path, dwElapsed);

    return true;
}

bool setNetworkAdapterState(ULONG ifIndex, bool enable)
{
    // Resolve ifIndex to the adapter's NetCfgInstanceId GUID via typed Win32 APIs so the
    // SetupDi enumeration below can be matched without any client-supplied string entering
    // the lookup. This is the LPE-relevant property of this function: the caller picks an
    // index, the helper picks the device.
    NET_LUID luid = {};
    DWORD ret = ::ConvertInterfaceIndexToLuid(ifIndex, &luid);
    if (ret != NO_ERROR) {
        spdlog::error("setNetworkAdapterState: ConvertInterfaceIndexToLuid failed for ifIndex={} ({})", ifIndex, ret);
        return false;
    }

    GUID interfaceGuid = {};
    ret = ::ConvertInterfaceLuidToGuid(&luid, &interfaceGuid);
    if (ret != NO_ERROR) {
        spdlog::error("setNetworkAdapterState: ConvertInterfaceLuidToGuid failed for ifIndex={} ({})", ifIndex, ret);
        return false;
    }

    // NetCfgInstanceId is stored as the canonical "{GUID}" form, so format the same way for comparison.
    wchar_t targetGuidStr[40] = {};
    if (::StringFromGUID2(interfaceGuid, targetGuidStr, _countof(targetGuidStr)) == 0) {
        spdlog::error("setNetworkAdapterState: StringFromGUID2 failed for ifIndex={}", ifIndex);
        return false;
    }

    // Restrict the enumeration to the Net device class and present devices only — we never
    // want to PROPERTYCHANGE an absent/ghost adapter or anything outside the network class.
    HDEVINFO hDevInfo = ::SetupDiGetClassDevsW(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        spdlog::error("setNetworkAdapterState: SetupDiGetClassDevs failed: {}", ::GetLastError());
        return false;
    }
    auto devInfoCleanup = wsl::wsScopeGuard([&]() { ::SetupDiDestroyDeviceInfoList(hDevInfo); });

    SP_DEVINFO_DATA devData = {};
    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    bool found = false;
    for (DWORD i = 0; ::SetupDiEnumDeviceInfo(hDevInfo, i, &devData); ++i) {
        HKEY hKey = ::SetupDiOpenDevRegKey(hDevInfo, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE) {
            continue;
        }

        wchar_t netCfgId[64] = {};
        // RegQueryValueEx does not guarantee a REG_SZ value is null-terminated. Reserve a wchar_t
        // of headroom the API can never write into: combined with the zero-init, that keeps
        // netCfgId terminated for the _wcsicmp below. (An over-long value returns ERROR_MORE_DATA
        // and is skipped.)
        DWORD bufSize = sizeof(netCfgId) - sizeof(wchar_t);
        DWORD valueType = 0;
        LONG result = ::RegQueryValueExW(hKey, L"NetCfgInstanceId", NULL, &valueType,
                                         reinterpret_cast<LPBYTE>(netCfgId), &bufSize);
        ::RegCloseKey(hKey);

        if (result != ERROR_SUCCESS || valueType != REG_SZ) {
            continue;
        }

        if (_wcsicmp(netCfgId, targetGuidStr) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        spdlog::error(L"setNetworkAdapterState: no Net-class device matched NetCfgInstanceId {}", targetGuidStr);
        return false;
    }

    SP_PROPCHANGE_PARAMS params = {};
    params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    params.StateChange = enable ? DICS_ENABLE : DICS_DISABLE;
    params.Scope = DICS_FLAG_GLOBAL;
    params.HwProfile = 0;

    if (!::SetupDiSetClassInstallParamsW(hDevInfo, &devData, &params.ClassInstallHeader, sizeof(params))) {
        spdlog::error("setNetworkAdapterState: SetupDiSetClassInstallParams failed for ifIndex={} ({})", ifIndex, ::GetLastError());
        return false;
    }

    if (!::SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, &devData)) {
        spdlog::error("setNetworkAdapterState: SetupDiCallClassInstaller failed for ifIndex={} ({})", ifIndex, ::GetLastError());
        return false;
    }

    return true;
}

GUID guidFromString(const std::wstring &str)
{
    GUID reqGUID;
    unsigned long p0;
    unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

    swscanf_s(str.c_str(), L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
              &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    reqGUID.Data1 = p0;
    reqGUID.Data2 = static_cast<unsigned short>(p1);
    reqGUID.Data3 = static_cast<unsigned short>(p2);
    reqGUID.Data4[0] = static_cast<unsigned char>(p3);
    reqGUID.Data4[1] = static_cast<unsigned char>(p4);
    reqGUID.Data4[2] = static_cast<unsigned char>(p5);
    reqGUID.Data4[3] = static_cast<unsigned char>(p6);
    reqGUID.Data4[4] = static_cast<unsigned char>(p7);
    reqGUID.Data4[5] = static_cast<unsigned char>(p8);
    reqGUID.Data4[6] = static_cast<unsigned char>(p9);
    reqGUID.Data4[7] = static_cast<unsigned char>(p10);

    return reqGUID;
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

        dwFwApiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &id);
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

        dwFwApiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &id);
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
        DWORD dwFwApiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &id);
        if (dwFwApiRetCode != ERROR_SUCCESS) {
            spdlog::error("Error adding ICMPv6 type filter: {}", dwFwApiRetCode);
            success = false;
        }
    }
    return success;
}

std::wstring getConfigPath()
{
    // To prevent shenanigans with various TOCTOU exploits, the config file should be in Program Files,
    // which is only writable by administrators
    wchar_t* programFilesPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &programFilesPath);
    if (FAILED(hr)) {
        spdlog::error("Failed to get Program Files dir");
        return L"";
    }

    std::wstringstream filePath;
    filePath << programFilesPath;
    CoTaskMemFree(programFilesPath);
    filePath << L"\\" WS_WIN_CONFIG_SUBDIR_W L"\\config";
    int ret = SHCreateDirectoryEx(NULL, filePath.str().c_str(), NULL);
    if (ret != ERROR_SUCCESS && ret != ERROR_ALREADY_EXISTS) {
        spdlog::error("Failed to create config dir");
        return L"";
    }

    return filePath.str();
}

std::wstring getUpdatePath(bool create)
{
    // To prevent shenanigans with various TOCTOU exploits, the staged installer should be in Program Files,
    // which is only writable by administrators
    wchar_t* programFilesPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &programFilesPath);
    if (FAILED(hr)) {
        spdlog::error("Failed to get Program Files dir");
        return L"";
    }

    std::wstringstream filePath;
    filePath << programFilesPath;
    CoTaskMemFree(programFilesPath);
    filePath << L"\\" WS_WIN_CONFIG_SUBDIR_W L"\\update";
    if (create) {
        int ret = SHCreateDirectoryEx(NULL, filePath.str().c_str(), NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_ALREADY_EXISTS) {
            spdlog::error("Failed to create update dir");
            return L"";
        }
    }

    return filePath.str();
}

std::wstring getSystemDir()
{
    wchar_t path[MAX_PATH];
    UINT result = ::GetSystemDirectory(path, MAX_PATH);
    if (result == 0 || result >= MAX_PATH) {
        spdlog::error("GetSystemDir failed ({})", ::GetLastError());
        return std::wstring(L"C:\\Windows\\System32");
    }

    return std::wstring(path);
}

std::string ssidFromInterfaceGUID(const std::wstring &interfaceGUID)
{
    // This DLL is not available on default installs of Windows Server.  Dynamically load it so
    // the app doesn't fail to launch with a "DLL not found" error.  App profiling was performed
    // and indicated no performance degradation when dynamically loading and unloading the DLL.
    const std::wstring dll = getSystemDir() + L"\\wlanapi.dll";
    auto wlanDll = ::LoadLibraryEx(dll.c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (wlanDll == NULL) {
        throw std::system_error(::GetLastError(), std::generic_category(), "wlanapi.dll could not be loaded");
    }

    auto freeDLL = wsl::wsScopeGuard([&] {
        ::FreeLibrary(wlanDll);
    });

    typedef DWORD (WINAPI * WlanOpenHandleFunc)(DWORD dwClientVersion, PVOID pReserved, PDWORD pdwNegotiatedVersion, PHANDLE phClientHandle);
    typedef DWORD (WINAPI * WlanCloseHandleFunc)(HANDLE hClientHandle, PVOID pReserved);
    typedef VOID  (WINAPI * WlanFreeMemoryFunc)(PVOID pMemory);
    typedef DWORD (WINAPI * WlanQueryInterfaceFunc)(HANDLE hClientHandle, CONST GUID *pInterfaceGuid, WLAN_INTF_OPCODE OpCode, PVOID pReserved,
                                                   PDWORD pdwDataSize, PVOID *ppData, PWLAN_OPCODE_VALUE_TYPE pWlanOpcodeValueType);

    WlanOpenHandleFunc pfnWlanOpenHandle = (WlanOpenHandleFunc)::GetProcAddress(wlanDll, "WlanOpenHandle");
    if (pfnWlanOpenHandle == NULL) {
        throw std::system_error(::GetLastError(), std::generic_category(), "Failed to load WlanOpenHandle");
    }

    WlanCloseHandleFunc pfnWlanCloseHandle = (WlanCloseHandleFunc)::GetProcAddress(wlanDll, "WlanCloseHandle");
    if (pfnWlanCloseHandle == NULL) {
        throw std::system_error(::GetLastError(), std::generic_category(), "Failed to load WlanCloseHandle");
    }

    WlanFreeMemoryFunc pfnWlanFreeMemory = (WlanFreeMemoryFunc)::GetProcAddress(wlanDll, "WlanFreeMemory");
    if (pfnWlanFreeMemory == NULL) {
        throw std::system_error(::GetLastError(), std::generic_category(), "Failed to load WlanFreeMemory");
    }

    WlanQueryInterfaceFunc pfnWlanQueryInterface = (WlanQueryInterfaceFunc)::GetProcAddress(wlanDll, "WlanQueryInterface");
    if (pfnWlanQueryInterface == NULL) {
        throw std::system_error(::GetLastError(), std::generic_category(), "Failed to load WlanQueryInterface");
    }

    DWORD dwCurVersion = 0;
    HANDLE hClient = NULL;
    auto result = pfnWlanOpenHandle(2, NULL, &dwCurVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        throw std::system_error(result, std::generic_category(), "WlanOpenHandle failed");
    }

    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;

    auto freeWlanResources = wsl::wsScopeGuard([&] {
        if (pConnectInfo != NULL) {
            pfnWlanFreeMemory(pConnectInfo);
        }

        pfnWlanCloseHandle(hClient, NULL);
    });

    GUID actualGUID = guidFromString(interfaceGUID);

    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

    result = pfnWlanQueryInterface(hClient, &actualGUID, wlan_intf_opcode_current_connection, NULL,
                                   &connectInfoSize, (PVOID *) &pConnectInfo, &opCode);
    if (result != ERROR_SUCCESS) {
        throw std::system_error(result, std::generic_category(), "WlanQueryInterface failed");
    }

    std::string ssid;
    const auto &dot11Ssid = pConnectInfo->wlanAssociationAttributes.dot11Ssid;
    if (dot11Ssid.uSSIDLength > 0) {
        ssid.reserve(dot11Ssid.uSSIDLength);
        for (ULONG k = 0; k < dot11Ssid.uSSIDLength; k++) {
            ssid.push_back(static_cast<char>(dot11Ssid.ucSSID[k]));
        }
    }

    return ssid;
}

void debugOut(const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    char szMsg[1024];
    _vsnprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    ::OutputDebugStringA(szMsg);
}

static BOOL isElevated()
{
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

void disableFirewall()
{
    if (isElevated()) {
        FwpmWrapper fwpmWrapper;
        if (fwpmWrapper.initialize()) {
            Ipv6Firewall::instance(&fwpmWrapper).enableIPv6();
            DnsFirewall::instance(&fwpmWrapper).disable();
            FirewallFilter::instance(&fwpmWrapper).off();
            SplitTunneling::removeAllFilters(fwpmWrapper);
            printf(WS_PRODUCT_NAME " firewall deleted.\n");
        } else {
            printf("Failed to initialize access to the Windows firewall manager.\n");
        }
    } else {
        printf("Please run the program with administrator rights.\n");
    }
}

}
