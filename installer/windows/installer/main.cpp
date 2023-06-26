#include <Windows.h>
#include <tchar.h>
#include <versionhelpers.h>

#include <algorithm>
#include <regex>

#include "gui/application.h"
#include "installer/settings.h"
#include "../utils/applicationinfo.h"
#include "../utils/path.h"
#include "../utils/logger.h"

// Set the DLL load directory to the system directory before entering WinMain().
struct LoadSystemDLLsFromSystem32
{
    LoadSystemDLLsFromSystem32()
    {
        // Remove the current directory from the search path for dynamically loaded
        // DLLs as a precaution.  This call has no effect for delay load DLLs.
        SetDllDirectory(L"");
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
} loadSystemDLLs;

namespace
{
int argCount = 0;
LPWSTR *argList = nullptr;

static int WSMessageBox(HWND hOwner, LPCTSTR szTitle, UINT nStyle, LPCTSTR szFormat, ...)
{
    va_list arg_list;
    va_start(arg_list, szFormat);

    TCHAR Buf[1024];
    _vsntprintf(Buf, 1023, szFormat, arg_list);
    Buf[1023] = _T('\0');

    va_end(arg_list);

    int nResult = ::MessageBox(hOwner, Buf, szTitle, nStyle);

    return nResult;
}

// This function cannot handle negative coordinates passed with the -center option; -dir option seems to break sometimes too
// For now (we can make this better later) we assume 2 additional args will be passed with -center and 1 for -dir
static bool CheckCommandLineArgument(LPCWSTR argumentToCheck, int *valueIndex = nullptr,
                                     int *valueCount = nullptr)
{
    if (!argumentToCheck)
        return false;

    if (!argList)
        argList = CommandLineToArgvW(GetCommandLine(), &argCount);

    bool result = false;
    for (int i = 0; i < argCount; ++i) {
        if (wcscmp(argList[i], argumentToCheck))
            continue;
        result = true;
        if (!valueCount)
            continue;
        *valueCount = 0;
        if (valueIndex)
            *valueIndex = -1;
        for (int j = i + 1; j < argCount; ++j) {
            if (wcsncmp(argList[j], L"-", 1)) {
                if (valueIndex && *valueIndex < 0)
                    *valueIndex = j;
                ++*valueCount;
            }
            else
            {
                break;
            }
        }
    }
    return result;
}

static bool GetCommandLineArgumentIndex(LPCWSTR argumentToCheck, int *valueIndex)
{
    if (!argumentToCheck)
        return false;

    if (!argList)
        argList = CommandLineToArgvW(GetCommandLine(), &argCount);

    bool result = false;
    for (int i = 0; i < argCount; ++i) {
        if (wcscmp(argList[i], argumentToCheck))
            continue;
        result = true;

        *valueIndex = i + 1;
    }
    return result;
}

static void replaceAll(std::wstring &inout, std::wstring_view what, std::wstring_view with)
{
    for (std::wstring::size_type pos{}; inout.npos != (pos = inout.find(what.data(), pos, what.length())); pos += with.length()) {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
}

static std::wstring sanitizedCommandLine(const std::wstring &exeName, const std::wstring &username, const std::wstring &password)
{
    std::wstring cmdLine(::GetCommandLine());
    if (!exeName.empty()) {
        replaceAll(cmdLine, argList[0], exeName);
    }

    if (!username.empty()) {
        std::wstring oldVal = L"\"" + username + L"\"";
        replaceAll(cmdLine, oldVal, std::wstring(L"\"********\""));
    }

    if (!password.empty()) {
        std::wstring oldVal = L"\"" + password + L"\"";
        replaceAll(cmdLine, oldVal, std::wstring(L"\"********\""));
    }

    return cmdLine;
}

}  // namespace


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    if (!IsWindows10OrGreater())
    {
        WSMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
            _T("The Windscribe app can only be installed on Windows 10 or newer."));
        return 0;
    }

#if !defined(_M_ARM64)
    {
        // Tried using GetSystemInfo and GetNativeSystemInfo.  Both return PROCESSOR_ARCHITECTURE_AMD64
        // when run on Windows 11 ARM64.
        USHORT process_machine;
        USHORT native_machine;
        BOOL result = ::IsWow64Process2(::GetCurrentProcess(), &process_machine, &native_machine);
        if (result && native_machine == IMAGE_FILE_MACHINE_ARM64) {
            WSMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                         _T("This version of the Windscribe app will not operate correctly on your PC."
                            "  Please download the 'ARM64' version from the Windscribe website to ensure"
                            " optimal compatibility and performance."));
            return 0;
        }
    }
#endif

    if (CheckCommandLineArgument(L"-help"))
    {
        WSMessageBox(NULL, _T("Windscribe Install Options"), MB_OK | MB_ICONINFORMATION,
            _T("The Windscribe installer accepts the following optional commmand-line parameters:\n\n"
               "-help\n"
                "Show this information.\n\n"
                "-no-auto-start\n"
                "Do not launch the application after installation.\n\n"
                "-no-drivers\n"
                "Instructs the installer to skip installing drivers.\n\n"
                "-silent\n"
                "Instructs the installer to hide its user interface.  Implies -no-drivers and -no-auto-start.\n\n"
                "-factory-reset\n"
                "Delete existing preferences, logs, and other data, if they exist.\n\n"
                "-dir \"C:\\dirname\"\n"
                "Overrides the default installation directory. Installation directory must be on the system drive.\n\n"
                "-username \"my username\"\n"
                "Sets the username the application will use to automatically log in when first launched.\n\n"
                "-password \"my password\"\n"
                "Sets the password the application will use to automatically log in when first launched."));
        return 0;
    }

    // Useful for debugging
    // AllocConsole();
    // FILE *stream;
    // freopen_s(&stream, "CONOUT$", "w+t", stdout);

    const bool isUpdateMode = CheckCommandLineArgument(L"-update") || CheckCommandLineArgument(L"-q");
    int expectedArgumentCount = isUpdateMode ? 2 : 1;

    int window_center_x = -1, window_center_y = -1;
    int center_coord_index = 0;
    if (GetCommandLineArgumentIndex(L"-center", &center_coord_index)) {
            window_center_x = _wtoi(argList[center_coord_index]);
            window_center_y = _wtoi(argList[center_coord_index+1]);
        expectedArgumentCount += 3;
    }

    bool isSilent = false;
    bool noDrivers = false;
    bool noAutoStart = false;
    bool isFactoryReset = false;
    std::wstring installPath;
    std::wstring username;
    std::wstring password;

    if (!isUpdateMode)
    {
        isSilent = CheckCommandLineArgument(L"-silent");
        if (isSilent) {
            expectedArgumentCount += 1;
        }
        noDrivers = CheckCommandLineArgument(L"-no-drivers");
        if (noDrivers) {
            expectedArgumentCount += 1;
        }
        noAutoStart = CheckCommandLineArgument(L"-no-auto-start");
        if (noAutoStart) {
            expectedArgumentCount += 1;
        }
        isFactoryReset = CheckCommandLineArgument(L"-factory-reset");
        if (isFactoryReset) {
            expectedArgumentCount += 1;
        }

        int install_path_index = 0;
        if (GetCommandLineArgumentIndex(L"-dir", &install_path_index))
        {
            if (install_path_index > 0)
            {
                if (install_path_index >= argCount)
                {
                    WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
                        _T("The -dir parameter was specified but the directory path was not."));
                    return 0;
                }

                installPath = argList[install_path_index];
                std::replace(installPath.begin(), installPath.end(), L'/', L'\\');
                expectedArgumentCount += 2;
            }
        }

        int username_index = 0;
        if (GetCommandLineArgumentIndex(L"-username", &username_index))
        {
            if (username_index > 0)
            {
                if (username_index >= argCount)
                {
                    WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
                        _T("The -username parameter was specified but the username was not."));
                    return 0;
                }

                username = argList[username_index];
                expectedArgumentCount += 2;
            }
        }

        int password_index = 0;
        if (GetCommandLineArgumentIndex(L"-password", &password_index))
        {
            if (password_index > 0)
            {
                if (password_index >= argCount)
                {
                    WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
                        _T("The -password parameter was specified but the password was not."));
                    return 0;
                }

                password = argList[password_index];
                expectedArgumentCount += 2;
            }
        }
    }

    if (argCount != expectedArgumentCount)
    {
        WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
            _T("Incorrect number of arguments passed to installer.\n\nUse the -help argument to see available arguments and their format."));
        return 0;
    }

    if (!installPath.empty() && !Path::isOnSystemDrive(installPath)) {
        WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
            _T("The specified installation path is not on the system drive.  To ensure the security of the application, and your system, it must be installed on the same drive as Windows."));
        return 0;
    }

    if (!username.empty()) {
        if (password.empty()) {
            WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
                _T("A username was specified but its corresponding password was not provided.\n\nUse the -help argument to see available arguments and their format."));
            return 0;
        }

        if (username.find_first_of(L'@') != std::wstring::npos) {
            WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR, _T("Your username should not be an email address. Please try again."));
            return 0;
        }
    }

    if (!password.empty() && username.empty()) {
        WSMessageBox(NULL, _T("Windscribe Install Error"), MB_OK | MB_ICONERROR,
            _T("A password was specified but its corresponding username was not provided.\n\nUse the -help argument to see available arguments and their format."));
        return 0;
    }

    Log::instance().init(true);
    Log::instance().out(L"Installing Windscribe version " + ApplicationInfo::instance().getVersion());
    std::wstring exeName = argList[0];
    exeName = exeName.substr(exeName.find_last_of(L"/\\") + 1);
    Log::instance().out(L"Command-line args: " + sanitizedCommandLine(exeName, username, password));

    Application app(hInstance, nCmdShow, isUpdateMode, isSilent, noDrivers, noAutoStart, isFactoryReset, installPath, username, password);

    int result = -1;
    if (app.init(window_center_x, window_center_y)) {
        result = app.exec();
    }

    Log::instance().writeFile(Settings::instance().getPath());

    return result;
}
