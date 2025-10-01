#include "winutils.h"

#include <QSettings>

#include <LM.h>
#include <psapi.h>
#include <filesystem>
#include <shellapi.h>

#include "types/global_consts.h"
#include "log/categories.h"
#include "servicecontrolmanager.h"
#include "win32handle.h"

#define MAX_KEY_LENGTH 255

bool WinUtils::reboot()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }

    // Get the LUID for the shutdown privilege.
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process.
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS)
    {
        return false;
    }

    // Shut down the system and force all applications to close.
    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED))
    {
        return false;
    }

    return true;
}

static bool getWinVersion(RTL_OSVERSIONINFOEXW *rtlOsVer)
{
    NTSTATUS (WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");
    if (RtlGetVersion)
    {
        memset(rtlOsVer, 0, sizeof(RTL_OSVERSIONINFOEXW));
        rtlOsVer->dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
        RtlGetVersion(rtlOsVer);
        return true;
    }
    return false;
}

bool WinUtils::isWindows10orGreater()
{
    RTL_OSVERSIONINFOEXW rtlOsVer;
    if (getWinVersion(&rtlOsVer))
    {
        if (rtlOsVer.dwMajorVersion >= 10)
        {
            return true;
        }
    }
    return false;
}

bool WinUtils::isDohSupported()
{
    RTL_OSVERSIONINFOEXW rtlOsVer;
    if (getWinVersion(&rtlOsVer)) {
        return rtlOsVer.dwMajorVersion >= 10 && rtlOsVer.dwBuildNumber >= 19628;
    }

    return false;
}

QString WinUtils::getWinVersionString()
{
    RTL_OSVERSIONINFOEXW rtlOsVer;
    if (!getWinVersion(&rtlOsVer)) {
        return "Failed to detect Windows version";
    }

    QString ret;
    if (rtlOsVer.dwMajorVersion == 10 && rtlOsVer.dwMinorVersion >= 0 && rtlOsVer.wProductType == VER_NT_WORKSTATION) ret = (rtlOsVer.dwBuildNumber >= kWindows11BuildNumber ? "Windows 11" : "Windows 10");
    else if (rtlOsVer.dwMajorVersion == 10 && rtlOsVer.dwMinorVersion >= 0 && rtlOsVer.wProductType != VER_NT_WORKSTATION) ret = "Windows 10 Server";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 3 && rtlOsVer.wProductType != VER_NT_WORKSTATION) ret = "Windows Server 2012 R2";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 3 && rtlOsVer.wProductType == VER_NT_WORKSTATION) ret = "Windows 8.1";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 2 && rtlOsVer.wProductType != VER_NT_WORKSTATION) ret = "Windows Server 2012";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 2 && rtlOsVer.wProductType == VER_NT_WORKSTATION) ret = "Windows 8";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 1 && rtlOsVer.wProductType != VER_NT_WORKSTATION) ret = "Windows Server 2008 R2";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 1 && rtlOsVer.wProductType == VER_NT_WORKSTATION) ret = "Windows 7";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 0 && rtlOsVer.wProductType != VER_NT_WORKSTATION) ret = "Windows Server 2008";
    else if (rtlOsVer.dwMajorVersion == 6 && rtlOsVer.dwMinorVersion == 0 && rtlOsVer.wProductType == VER_NT_WORKSTATION) ret = "Windows Vista";
    else if (rtlOsVer.dwMajorVersion == 5 && rtlOsVer.dwMinorVersion == 2 && rtlOsVer.wProductType == VER_NT_WORKSTATION) ret = "Windows XP";
    else if (rtlOsVer.dwMajorVersion == 5 && rtlOsVer.dwMinorVersion == 2) ret = "Windows Server 2003";
    else if (rtlOsVer.dwMajorVersion == 5 && rtlOsVer.dwMinorVersion == 1) ret = "Windows XP";
    else if (rtlOsVer.dwMajorVersion == 5 && rtlOsVer.dwMinorVersion == 0) ret = "Windows 2000";
    else ret = "Unknown";

    if (rtlOsVer.szCSDVersion[0] != L'\0') {
        ret += " " + QString::fromStdWString(rtlOsVer.szCSDVersion);
    }

    ret += " (major: " + QString::number(rtlOsVer.dwMajorVersion) + ", minor: " + QString::number(rtlOsVer.dwMinorVersion) + ")";
    ret += " (build: " + QString::number(rtlOsVer.dwBuildNumber);

    // Retrieve the Update Build Revision to aid us in determining if the user is running an insider preview.
    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", QSettings::NativeFormat);
    uint ubr = registry.value("UBR", 0).toUInt();
    if (ubr != 0) {
        ret += "." + QString::number(ubr);
    }

    ret += ")";

    return ret;
}

void WinUtils::getOSVersionAndBuild(QString &osVersion, QString &build)
{
    RTL_OSVERSIONINFOEXW rtlOsVer;
    if (getWinVersion(&rtlOsVer))
    {
        osVersion = QString::number(rtlOsVer.dwMajorVersion) + "." + QString::number(rtlOsVer.dwMinorVersion);
        build = QString::number(rtlOsVer.dwBuildNumber);
    }
    else
    {
        osVersion = "NotDetected";
        build = "NotDetected";
    }
}

QStringList WinUtils::enumerateSubkeyNames(HKEY rootKey, const QString &keyPath)
{
    QStringList result;

    std::wstring subKey = keyPath.toStdWString();

    HKEY hKeyParent;
    if (RegOpenKeyEx(rootKey,
                     subKey.c_str(),
                     0, KEY_READ, &hKeyParent) == ERROR_SUCCESS)
    {
        TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
        DWORD    cchClassName = MAX_PATH;  // size of class string
        DWORD    cSubKeys=0;               // number of subkeys
        DWORD    cbMaxSubKey;              // longest subkey size
        DWORD    cchMaxClass;              // longest class string
        DWORD    cValues;              // number of values for key
        DWORD    cchMaxValue;          // longest value name
        DWORD    cbMaxValueData;       // longest value data
        DWORD    cbSecurityDescriptor; // size of security descriptor
        FILETIME ftLastWriteTime;      // last write time

        DWORD retCode;

        // HKEY hKey;
        // Get the class name and the value count.
        retCode = RegQueryInfoKey(
            hKeyParent,                    // key handle
            achClass,                // buffer for class name
            &cchClassName,           // size of class string
            NULL,                    // reserved
            &cSubKeys,               // number of subkeys
            &cbMaxSubKey,            // longest subkey size
            &cchMaxClass,            // longest class string
            &cValues,                // number of values for this key
            &cchMaxValue,            // longest value name
            &cbMaxValueData,         // longest value data
            &cbSecurityDescriptor,   // security descriptor
            &ftLastWriteTime);       // last write time


        if (retCode == ERROR_SUCCESS && cSubKeys)
        {
            for (DWORD i=0; i<cSubKeys; i++)
            {
                TCHAR achKey[MAX_KEY_LENGTH];   // buffer for subkey name
                DWORD cbName = MAX_KEY_LENGTH;  // size of name string
                retCode = RegEnumKeyEx(
                    hKeyParent,
                    i,
                    achKey,
                    &cbName,
                    NULL,
                    NULL,
                    NULL,
                    &ftLastWriteTime);
                if (retCode == ERROR_SUCCESS)
                {
                    result.append(QString::fromWCharArray(achKey));
                }
            }
        }

        RegCloseKey(hKeyParent);
    }

    return result;
}

static QString regGetLocalMachineRegistryValueSz(HKEY rootKey, QString keyPath, QString propertyName, bool wow64)
{
    QString result = "";

    DWORD options = KEY_READ;
    if (wow64) options |= KEY_WOW64_64KEY;

    HKEY hKey;
    LONG nError = RegOpenKeyEx(rootKey, keyPath.toStdWString().c_str(), NULL, options, &hKey);
    if (nError == ERROR_SUCCESS)
    {
        DWORD dwType = REG_SZ;
        char value[1024];
        DWORD value_length = 1024;
        nError = RegQueryValueEx(hKey, propertyName.toStdWString().c_str(), NULL, &dwType, (LPBYTE) &value, &value_length);
        if(nError == ERROR_SUCCESS)
        {
            // TODO: make this better!
            for (DWORD i = 0; i < value_length; i++)
            {
                if (value[i] > 16)
                {
                    result += QString(value[i]);
                }
                else
                {
                    result += QString::fromUtf8(&value[i]);
                }
            }

            RegCloseKey(hKey);
        }
        else
        {
            RegCloseKey(hKey);
        }
    }

    return result;
}

static QMap<QString,QString> enumerateCleanedProgramLocations(HKEY hKey, QString keyPath, bool wow64)
{
    QMap<QString, QString> programLocations;

    const QStringList uninstallSubkeys = WinUtils::enumerateSubkeyNames(hKey, keyPath);

    for (const QString &subkey : uninstallSubkeys)
    {
        QString fullKeyName = keyPath + "\\" + subkey;
        QString iconPath = regGetLocalMachineRegistryValueSz(hKey, fullKeyName,"DisplayIcon", wow64);
        QString programName = regGetLocalMachineRegistryValueSz(hKey, fullKeyName,"DisplayName", wow64);

        if (iconPath != "" && programName != "")
        {
            if (iconPath.contains("\"")) iconPath = iconPath.remove("\"");
            int lastIndex = iconPath.indexOf(",");

            QString filteredIconPath = iconPath.mid(0, lastIndex);
            programLocations.insert(programName, filteredIconPath);
        }
    }

    return programLocations;
}

QMap<QString, QString> WinUtils::enumerateInstalledProgramIconLocations()
{
    QMap<QString, QString> programLocations;

    QString keyPath("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");

    QList<QMap<QString,QString>> maps;
    maps.append(enumerateCleanedProgramLocations(HKEY_LOCAL_MACHINE, keyPath, true)); // HLM 64
    maps.append(enumerateCleanedProgramLocations(HKEY_LOCAL_MACHINE, keyPath, false)); // HLM 32
    maps.append(enumerateCleanedProgramLocations(HKEY_CURRENT_USER, keyPath, false)); // HCU (32)

    for (int i = 0; i < maps.length(); i++)
    {
        const QMap<QString,QString> map = maps[i];
        const auto mapKeys = map.keys();
        for (const QString &name : mapKeys)
        {
            programLocations.insert(name, map[name]);
        }
    }
    return programLocations;
}

QString WinUtils::regGetLocalMachineRegistryValueSz(QString keyPath, QString propertyName)
{
    QString result = "";

    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath.toStdWString().c_str(), NULL, KEY_READ, &hKey);
    if (nError == ERROR_SUCCESS)
    {
        DWORD dwType = REG_SZ;
        char value[1024];
        DWORD value_length = 1024;
        nError = RegQueryValueEx(hKey, propertyName.toStdWString().c_str(), NULL, &dwType, (LPBYTE) &value, &value_length);
        if(nError == ERROR_SUCCESS)
        {
            // TODO: make this better!
            for (DWORD i = 0; i < value_length; i++)
            {
                if (value[i] > 16)
                {
                    result += QString(value[i]);
                }
                else
                {
                    result += QString::fromUtf8(&value[i]);
                }
            }

            RegCloseKey(hKey);
        }
        else
        {
            RegCloseKey(hKey);
        }
    }

    return result;
}

bool WinUtils::regHasLocalMachineSubkeyProperty(QString keyPath, QString propertyName)
{
    bool result = false;

    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath.toStdWString().c_str(), NULL, KEY_READ, &hKey);

    if (nError == ERROR_SUCCESS)
    {
        nError = RegQueryValueEx(hKey, propertyName.toStdWString().c_str(), nullptr, REG_NONE, nullptr, nullptr); // contents not required

        if(nError == ERROR_SUCCESS)
        {
            result = true;
        }

        RegCloseKey(hKey);
    }

    return result;
}

bool WinUtils::regGetCurrentUserRegistryDword(QString keyPath, QString propertyName, int &dwordValue)
{
    bool result = false;
    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, keyPath.toStdWString().c_str(),
                               NULL, KEY_READ, &hKey);
    if (nError == ERROR_SUCCESS)
    {
        DWORD dwBufSize = sizeof(dwordValue);
        nError = RegQueryValueEx(hKey, propertyName.toStdWString().c_str(),
                                 nullptr, nullptr, (LPBYTE) &dwordValue, &dwBufSize);

        if (nError == ERROR_SUCCESS)
        {
            result = true;
        }

        RegCloseKey(hKey);
    }

    return result;
}

static QString processExecutablePath(DWORD processID)
{
    QString result;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    // Get the process name.
    if (NULL != hProcess ) {
        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
        if (GetModuleFileNameEx(hProcess, NULL, szProcessName, sizeof(szProcessName)/sizeof(TCHAR))) {
            result = QString::fromWCharArray(szProcessName);
        }
    }

    CloseHandle(hProcess);

    return result;
}

QStringList WinUtils::enumerateRunningProgramLocations(bool allowDuplicate)
{
    QStringList result;

    // Get the list of process identifiers.
    DWORD aProcesses[1024], cbNeeded;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return result;
    }

    // Calculate how many process identifiers were returned.
    DWORD cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.
    for (auto i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            QString exePath = processExecutablePath(aProcesses[i]);
            if (!exePath.isEmpty() && (allowDuplicate || !result.contains(exePath))) {
                result.append(exePath);
            }
        }
    }

    return result;
}

static std::string readAllFromPipe(HANDLE hPipe)
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

QString WinUtils::executeBlockingCmd(QString cmd, const QString & /*params*/, int timeoutMs)
{
    QString result = "";

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa,sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE rPipe, wPipe;
    if (!CreatePipe(&rPipe, &wPipe, &sa, 0))
    {
        return result;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = wPipe;
    si.hStdOutput = wPipe;

    ZeroMemory( &pi, sizeof(pi) );
    if (CreateProcess(NULL, (wchar_t *) cmd.toStdWString().c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        DWORD exitCode;
        if (timeoutMs < 0)
        {
            exitCode = WaitForSingleObject(pi.hProcess, INFINITE);
        }
        else
        {
            exitCode = WaitForSingleObject(pi.hProcess, timeoutMs);
        }

        CloseHandle(wPipe);
        std::string str;
        str = readAllFromPipe(rPipe);
        CloseHandle(rPipe);
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );

        result = QString::fromStdString(str);
    }
    else
    {
        qCCritical(LOG_BASIC) << "Failed create process: %x", GetLastError();
        CloseHandle(rPipe);
        CloseHandle(wPipe);
    }
    return result;
}

bool WinUtils::isServiceRunning(const QString &serviceName)
{
    DWORD dwStatus = SERVICE_STOPPED;
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(serviceName.toStdWString().c_str(), SERVICE_QUERY_STATUS);
        dwStatus = scm.queryServiceStatus();
    }
    catch (std::system_error& ex) {
        qCWarning(LOG_BASIC) << "WinUtils::isServiceRunning -" << ex.what();
    }

    return (dwStatus == SERVICE_RUNNING);
}

unsigned long WinUtils::Win32GetErrorString(unsigned long errorCode, wchar_t *buffer, unsigned long bufferSize)
{
    DWORD nLength = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM    |
                                    FORMAT_MESSAGE_IGNORE_INSERTS |
                                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                    NULL, errorCode,
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                    buffer, bufferSize, NULL);
    if (nLength == 0) {
        nLength = _snwprintf_s(buffer, bufferSize, _TRUNCATE, L"Unknown error code [%lu].", errorCode);
    }

    return nLength;
}

QString WinUtils::getVersionInfoItem(QString exeName, QString itemName)
{
    QString itemValue;

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;

    std::wstring binary(exeName.toStdWString());

    DWORD dwNotUsed;
    DWORD dwSize = ::GetFileVersionInfoSize(binary.c_str(), &dwNotUsed);

    if (dwSize > 0) {
        std::unique_ptr<UCHAR[]> versionInfo(new UCHAR[dwSize]);
        BOOL bResult = ::GetFileVersionInfo(binary.c_str(), 0L, dwSize, versionInfo.get());
        if (bResult) {
            // To get a string value must pass query in the form
            // "\StringFileInfo\<langID><codepage>\keyname"
            // where <langID><codepage> is the languageID concatenated with the code page, in hex.
            UINT nTranslateLen;
            bResult = ::VerQueryValue(versionInfo.get(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &nTranslateLen);

            if (bResult) {
                for (UINT i = 0; i < (nTranslateLen/sizeof(struct LANGANDCODEPAGE)); ++i)
                {
                    wchar_t subBlock[MAX_PATH];
                    swprintf_s(subBlock, MAX_PATH, L"\\StringFileInfo\\%04x%04x\\%s", lpTranslate[i].wLanguage,
                               lpTranslate[i].wCodePage, itemName.toStdWString().c_str());

                    LPVOID lpvi;
                    UINT   nLen;
                    bResult = ::VerQueryValue(versionInfo.get(), subBlock, &lpvi, &nLen);

                    if (bResult && nLen > 0) {
                        itemValue = QString::fromStdWString(std::wstring((LPCTSTR)lpvi));
                        break;
                    }
                }
            }
        }
    }

    return itemValue;
}

GUID WinUtils::stringToGuid(const char *str)
{
    GUID guid;
    unsigned long p0;
    unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

    sscanf_s(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);

    guid.Data1 = p0;
    guid.Data2 = p1;
    guid.Data3 = p2;
    guid.Data4[0] = p3;
    guid.Data4[1] = p4;
    guid.Data4[2] = p5;
    guid.Data4[3] = p6;
    guid.Data4[4] = p7;
    guid.Data4[5] = p8;
    guid.Data4[6] = p9;
    guid.Data4[7] = p10;

    return guid;
}

class EnumWindowInfo
{
public:
    HWND appMainWindow = nullptr;
};

static BOOL CALLBACK
FindAppWindowHandleProc(HWND hwnd, LPARAM lParam)
{
    DWORD processID = 0;
    ::GetWindowThreadProcessId(hwnd, &processID);

    if (processID == 0) {
        qCInfo(LOG_BASIC) << "FindAppWindowHandleProc GetWindowThreadProcessId failed" << ::GetLastError();
        return TRUE;
    }

    wsl::Win32Handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID));
    if (!hProcess.isValid()) {
        if (::GetLastError() != ERROR_ACCESS_DENIED) {
            qCCritical(LOG_BASIC) << "FindAppWindowHandleProc OpenProcess failed" << ::GetLastError();
        }
        return TRUE;
    }

    TCHAR imageName[MAX_PATH];
    DWORD pathLen = sizeof(imageName) / sizeof(imageName[0]);
    BOOL result = ::QueryFullProcessImageName(hProcess.getHandle(), 0, imageName, &pathLen);

    if (result == FALSE) {
        qCCritical(LOG_BASIC) << "FindAppWindowHandleProc QueryFullProcessImageName failed" << ::GetLastError();
        return TRUE;
    }

    std::filesystem::path path(std::wstring(imageName, pathLen));
    std::wstring exeName(path.filename());

    if (_wcsicmp(exeName.c_str(), L"windscribe.exe") == 0) {
        TCHAR buffer[128];
        int resultLen = ::GetWindowText(hwnd, buffer, sizeof(buffer) / sizeof(buffer[0]));

        if (resultLen > 0 && (_wcsicmp(buffer, L"windscribe") == 0)) {
            EnumWindowInfo* pWindowInfo = (EnumWindowInfo*)lParam;
            pWindowInfo->appMainWindow = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}

HWND WinUtils::appMainWindowHandle()
{
    auto pWindowInfo = std::make_unique<EnumWindowInfo>();
    ::EnumWindows((WNDENUMPROC)FindAppWindowHandleProc, (LPARAM)pWindowInfo.get());

    return pWindowInfo->appMainWindow;
}

bool WinUtils::isAppAlreadyRunning()
{
    auto handle = appMainWindowHandle();
    return handle != nullptr;
}

bool WinUtils::isOSCompatible()
{
    RTL_OSVERSIONINFOEXW rtlOsVer;
    if (getWinVersion(&rtlOsVer)) {
        return (rtlOsVer.dwMajorVersion >= 10 && rtlOsVer.dwBuildNumber >= kMinWindowsBuildNumber);
    }

    return true;
}

DWORD WinUtils::getOSBuildNumber()
{
    RTL_OSVERSIONINFOEXW rtlOsVer;
    if (getWinVersion(&rtlOsVer)) {
        return rtlOsVer.dwBuildNumber;
    }

    return 0;
}

QString WinUtils::getSystemDir()
{
    wchar_t path[MAX_PATH];
    UINT result = ::GetSystemDirectory(path, MAX_PATH);
    if (result == 0 || result >= MAX_PATH) {
        qCCritical(LOG_BASIC) << "GetSystemDirectory failed" << ::GetLastError();
        return QString("C:\\Windows\\System32");
    }

    return QString::fromWCharArray(path);
}

QStringList WinUtils::enumerateProcesses(const QString &processName)
{
    QStringList locations = enumerateRunningProgramLocations(true);
    QStringList result;

    for (const auto &path : locations) {
        if (path.endsWith(processName)) {
            result.append(path);
        }
    }

    return result;
}

bool WinUtils::isNotificationEnabled()
{
    int value = 0;

    if (regGetCurrentUserRegistryDword("Software\\Microsoft\\Windows\\CurrentVersion\\PushNotifications", "ToastEnabled", value)) {
        return value != 0;
    }

    return true; // Windows defaults to notifications enabled if this key does not exist
}

void WinUtils::openNotificationSettings()
{
    SHELLEXECUTEINFO shExInfo;
    memset(&shExInfo, 0, sizeof(shExInfo));

    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.hwnd = NULL;
    shExInfo.lpVerb = L"open";
    shExInfo.lpFile = L"ms-settings:notifications";
    shExInfo.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteEx(&shExInfo)) {
        qCWarning(LOG_BASIC) << "WinUtils::openNotificationSettings ShellExecuteEx failed" << ::GetLastError();
    }
}
