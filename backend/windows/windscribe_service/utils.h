#pragma once

#include <windows.h>
#include <ifdef.h>
#include "ip_address/ip4_address_and_mask.h"
#include "ip_address/ip6_address_and_prefix.h"
#include "apps_ids.h"

#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }
#define SAFE_RELEASE(x) if (x) { (x)->Release(); x = NULL; }

// Vista subnet mask
#define VISTA_SUBNET_MASK   0xffffffff

// Byte array IP address length
#define BYTE_IPADDR_ARRLEN    4

namespace Utils
{
    bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID);
    bool deleteAllFiltersForSublayer(HANDLE engineHandle, const GUID *subLayerGUID, GUID layerKey);

    std::string readAllFromPipe(HANDLE hPipe);
    std::wstring guidToStr(const GUID &guid);
    std::wstring getExePath();
    std::wstring getDirPathFromFullPath(std::wstring &fullPath);
    std::wstring getConfigPath();
    std::wstring getSystemDir();
    bool isValidFileName(std::wstring &filename);
    bool isFileExists(const wchar_t *path);
    bool hasWhitespaceInString(std::wstring &str);
    bool verifyWindscribeProcessPath(HANDLE hPipe);
    bool iequals(const std::wstring &a, const std::wstring &b);
    bool isMacAddress(const std::wstring &value);

    void callNetworkAdapterMethod(const std::wstring &methodName, const std::wstring &adapterRegistryName);
    GUID guidFromString(const std::wstring &str);

    bool addFilterV4(HANDLE engineHandle, std::vector<UINT64> *filterId, FWP_ACTION_TYPE type, UINT8 weight,
                     GUID subLayerKey, wchar_t *subLayerName, PNET_LUID pluid = nullptr,
                     const std::vector<Ip4AddressAndMask> *range = nullptr,
                     uint16_t localPort = 0, uint16_t remotePort = 0, AppsIds *appsIds = nullptr, bool persistent = true);
    bool addFilterV6(HANDLE engineHandle, std::vector<UINT64> *filterId, FWP_ACTION_TYPE type, UINT8 weight,
                     GUID subLayerKey, wchar_t *subLayerName, PNET_LUID pluid = nullptr,
                     const std::vector<Ip6AddressAndPrefix> *range = nullptr, bool persistent = true);

    std::string ssidFromInterfaceGUID(const std::wstring &interfaceGUID);

    void debugOut(const char* format, ...);
};
