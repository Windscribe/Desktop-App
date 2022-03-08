#pragma once

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
	void addPermitFilterIPv6ForAdapter(NET_IFINDEX tapInd, UINT8 weight, wchar_t *layerName, GUID &subLayerGUID, HANDLE hEngine);

	std::string readAllFromPipe(HANDLE hPipe);
	std::wstring guidToStr(const GUID &guid);
	std::wstring getExePath();
	std::wstring getDirPathFromFullPath(std::wstring &fullPath);
	bool isValidFileName(std::wstring &filename);
	bool isFileExists(const wchar_t *path);
	bool noSpacesInString(std::wstring &str);
	bool verifyWindscribeProcessPath(HANDLE hPipe);
    bool iequals(const std::wstring &a, const std::wstring &b);

	void callNetworkAdapterMethod(const std::wstring &methodName, const std::wstring &adapterRegistryName);
    GUID guidFromString(const std::wstring &str);
};
