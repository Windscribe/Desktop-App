#include "ws_branding.h"
#include "utils.h"

#include <Fwpmu.h>
#include <ws2def.h>
#include <Psapi.h>
#include <sddl.h>
#include <shlobj.h>
#include <wlanapi.h>
#include <tlhelp32.h>

#include <filesystem>
#include <sstream>

#include <spdlog/spdlog.h>

#include "dns_firewall.h"
#include "firewallfilter.h"
#include "fwpm_wrapper.h"
#include "ipv6_firewall.h"
#include "split_tunneling/split_tunneling.h"
#include "wfp_utils.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif
#include "utils/win32handle.h"
#include "utils/wsscopeguard.h"

namespace Utils
{

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
    const DWORD len = GetModuleFileName(NULL, buffer, MAX_PATH);
    if (len == 0) {
        spdlog::error("getExePath - GetModuleFileName failed: {}", ::GetLastError());
    } else if (len == MAX_PATH && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        spdlog::error("getExePath - GetModuleFileName truncated the path");
    }
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}

int terminateProcessesByPath(const std::wstring &exePath)
{
    const std::wstring exeName = std::filesystem::path(exePath).filename().wstring();

    wsl::Win32Handle snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot.isValid()) {
        spdlog::error("terminateProcessesByPath CreateToolhelp32Snapshot failed: {}", ::GetLastError());
        return 0;
    }

    int killed = 0;
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    for (BOOL more = ::Process32FirstW(snapshot.getHandle(), &entry); more; more = ::Process32NextW(snapshot.getHandle(), &entry)) {
        if (_wcsicmp(entry.szExeFile, exeName.c_str()) != 0) {
            continue;
        }

        wsl::Win32Handle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
                                               FALSE, entry.th32ProcessID));
        if (!process.isValid()) {
            spdlog::warn("terminateProcessesByPath OpenProcess({}) failed: {}", entry.th32ProcessID, ::GetLastError());
            continue;
        }

        // Only kill the binary at exePath, not a same-named exe from elsewhere.
        wchar_t imagePath[MAX_PATH * 2];
        DWORD imagePathLen = ARRAYSIZE(imagePath);
        if (!::QueryFullProcessImageNameW(process.getHandle(), 0, imagePath, &imagePathLen) ||
            !iequals(imagePath, exePath)) {
            continue;
        }

        if (!::TerminateProcess(process.getHandle(), 1)) {
            spdlog::error("terminateProcessesByPath TerminateProcess({}) failed: {}", entry.th32ProcessID, ::GetLastError());
            continue;
        }
        process.wait(5000);
        ++killed;
    }

    return killed;
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

static bool isSidStringSafeForSddl(const std::wstring &sid)
{
    // ConvertSidToStringSidW produces "S-1-5-..." with digits and hyphens only. Reject anything
    // else before interpolating into an SDDL string, so a ')' cannot close the ACE early.
    if (sid.size() < 3 || sid[0] != L'S' || sid[1] != L'-') {
        return false;
    }
    for (size_t i = 2; i < sid.size(); ++i) {
        const wchar_t c = sid[i];
        if (c != L'-' && (c < L'0' || c > L'9')) {
            return false;
        }
    }
    return true;
}

bool createRestrictedFileForUser(const std::wstring &path, PSID userSid)
{
    if (userSid == nullptr || !::IsValidSid(userSid)) {
        spdlog::error("createRestrictedFileForUser - invalid user SID");
        return false;
    }

    LPWSTR sidStr = nullptr;
    if (!::ConvertSidToStringSidW(userSid, &sidStr)) {
        spdlog::error("createRestrictedFileForUser - ConvertSidToStringSidW failed: {}", ::GetLastError());
        return false;
    }
    auto freeSidStr = wsl::wsScopeGuard([&] {
        ::LocalFree(sidStr);
    });

    const std::wstring sid(sidStr);
    if (!isSidStringSafeForSddl(sid)) {
        spdlog::error(L"createRestrictedFileForUser - refusing unsafe SID string: {}", sid);
        return false;
    }

    const std::wstring sddl = L"D:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FR;;;" + sid + L")";

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL)) {
        spdlog::error("createRestrictedFileForUser - failed to build security descriptor: {}", ::GetLastError());
        return false;
    }
    auto freeSD = wsl::wsScopeGuard([&] {
        ::LocalFree(sa.lpSecurityDescriptor);
    });

    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        spdlog::error("createRestrictedFileForUser - could not remove stale file: {}", ec.message());
        return false;
    }

    wsl::Win32Handle hFile(::CreateFileW(path.c_str(), GENERIC_WRITE, 0, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL));
    if (!hFile.isValid()) {
        spdlog::error("createRestrictedFileForUser - could not create file: {}", ::GetLastError());
        return false;
    }
    return true;
}

std::wstring getClientLogFolder(HANDLE userToken)
{
    // Qt's AppLocalDataLocation is FOLDERID_LocalAppData plus organization and application name.
    // Passing the pipe client's token keeps a SYSTEM helper from resolving its own profile.
    PWSTR localAppData = nullptr;
    const HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, userToken, &localAppData);
    if (FAILED(hr) || localAppData == nullptr) {
        ::CoTaskMemFree(localAppData);
        spdlog::error("getClientLogFolder - SHGetKnownFolderPath failed: {}", hr);
        return L"";
    }
    std::wstring folder = localAppData;
    ::CoTaskMemFree(localAppData);
    folder += L"\\" WS_SETTINGS_ORG_W L"\\" WS_SETTINGS_APP_W;
    return folder;
}

bool getPipeClientPrimaryToken(HANDLE hPipe, HANDLE *outToken)
{
    if (outToken == nullptr) {
        return false;
    }
    *outToken = nullptr;

    DWORD pidClient = 0;
    if (!::GetNamedPipeClientProcessId(hPipe, &pidClient) || pidClient == 0) {
        spdlog::error("getPipeClientPrimaryToken GetNamedPipeClientProcessId failed {}", ::GetLastError());
        return false;
    }

    DWORD clientSessionId = 0;
    if (!::ProcessIdToSessionId(pidClient, &clientSessionId)) {
        spdlog::error("getPipeClientPrimaryToken ProcessIdToSessionId failed {}", ::GetLastError());
        return false;
    }
    if (clientSessionId == 0) {
        spdlog::error("getPipeClientPrimaryToken rejecting client pid {} in Session 0", pidClient);
        return false;
    }

    wsl::Win32Handle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pidClient));
    if (!process.isValid()) {
        spdlog::error("getPipeClientPrimaryToken OpenProcess failed {}", ::GetLastError());
        return false;
    }

    wsl::Win32Handle processToken;
    if (!::OpenProcessToken(process.getHandle(), TOKEN_DUPLICATE | TOKEN_QUERY, processToken.data())) {
        spdlog::error("getPipeClientPrimaryToken OpenProcessToken failed {}", ::GetLastError());
        return false;
    }

    HANDLE primary = nullptr;
    if (!::DuplicateTokenEx(processToken.getHandle(),
                            TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID,
                            nullptr, SecurityImpersonation, TokenPrimary, &primary)) {
        spdlog::error("getPipeClientPrimaryToken DuplicateTokenEx failed {}", ::GetLastError());
        return false;
    }

    *outToken = primary;
    return true;
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

std::wstring getConfigPath()
{
    // To prevent shenanigans with various TOCTOU exploits, the config file should be in Program Files,
    // which is only writable by administrators
    wchar_t* programFilesPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &programFilesPath);
    if (FAILED(hr)) {
        spdlog::error("getConfigPath - Failed to get Program Files dir: {:#x}", static_cast<unsigned long>(hr));
        return L"";
    }

    std::wstringstream filePath;
    filePath << programFilesPath;
    CoTaskMemFree(programFilesPath);
    filePath << L"\\" WS_WIN_CONFIG_SUBDIR_W L"\\config";
    int ret = SHCreateDirectoryEx(NULL, filePath.str().c_str(), NULL);
    if (ret != ERROR_SUCCESS && ret != ERROR_ALREADY_EXISTS) {
        spdlog::error("getConfigPath - Failed to create config dir: {}", ret);
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
        spdlog::error("getUpdatePath - Failed to get Program Files dir: {:#x}", static_cast<unsigned long>(hr));
        return L"";
    }

    std::wstringstream filePath;
    filePath << programFilesPath;
    CoTaskMemFree(programFilesPath);
    filePath << L"\\" WS_WIN_CONFIG_SUBDIR_W L"\\update";
    if (create) {
        int ret = SHCreateDirectoryEx(NULL, filePath.str().c_str(), NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_ALREADY_EXISTS) {
            spdlog::error("getUpdatePath - Failed to create update dir: {}", ret);
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
        throw std::system_error(::GetLastError(), std::system_category(), "wlanapi.dll could not be loaded");
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
        throw std::system_error(::GetLastError(), std::system_category(), "Failed to load WlanOpenHandle");
    }

    WlanCloseHandleFunc pfnWlanCloseHandle = (WlanCloseHandleFunc)::GetProcAddress(wlanDll, "WlanCloseHandle");
    if (pfnWlanCloseHandle == NULL) {
        throw std::system_error(::GetLastError(), std::system_category(), "Failed to load WlanCloseHandle");
    }

    WlanFreeMemoryFunc pfnWlanFreeMemory = (WlanFreeMemoryFunc)::GetProcAddress(wlanDll, "WlanFreeMemory");
    if (pfnWlanFreeMemory == NULL) {
        throw std::system_error(::GetLastError(), std::system_category(), "Failed to load WlanFreeMemory");
    }

    WlanQueryInterfaceFunc pfnWlanQueryInterface = (WlanQueryInterfaceFunc)::GetProcAddress(wlanDll, "WlanQueryInterface");
    if (pfnWlanQueryInterface == NULL) {
        throw std::system_error(::GetLastError(), std::system_category(), "Failed to load WlanQueryInterface");
    }

    DWORD dwCurVersion = 0;
    HANDLE hClient = NULL;
    auto result = pfnWlanOpenHandle(2, NULL, &dwCurVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        throw std::system_error(result, std::system_category(), "WlanOpenHandle failed");
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
        throw std::system_error(result, std::system_category(), "WlanQueryInterface failed");
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
        } else {
            spdlog::error("isElevated - GetTokenInformation failed: {}", ::GetLastError());
        }
    } else {
        spdlog::error("isElevated - OpenProcessToken failed: {}", ::GetLastError());
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
            // Last, once no filter references it (otherwise this fails with FWP_E_IN_USE and the
            // provider stays, which is harmless).
            HANDLE hEngine = fwpmWrapper.getHandleAndLock();
            deleteWfpProvider(hEngine);
            fwpmWrapper.unlock();
            printf(WS_PRODUCT_NAME " firewall deleted.\n");
        } else {
            printf("Failed to initialize access to the Windows firewall manager.\n");
        }
    } else {
        printf("Please run the program with administrator rights.\n");
    }
}

}
