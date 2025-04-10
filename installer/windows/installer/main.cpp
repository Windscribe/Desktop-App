#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/callback_sink.h>

#include "installer/settings.h"
#include "mainwindow.h"
#include "options.h"
#include "languagesutil.h"
#include "../utils/applicationinfo.h"
#include "../../../client/common/utils/log/spdlog_utils.h"
#include "../utils/path.h"
#include "win32handle.h"

#include "../../../libs/wssecure/wssecure_globals.h"

namespace
{
int argCount = 0;
LPWSTR *argList = nullptr;

static std::optional<bool> isElevated()
{
    wsl::Win32Handle token;
    BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.data());
    if (result == FALSE) {
        spdlog::error(L"isElevated - OpenProcessToken failed: {}", GetLastError());
        return std::nullopt;
    }

    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof(TOKEN_ELEVATION);
    result = GetTokenInformation(token.getHandle(), TokenElevation, &Elevation, sizeof(Elevation), &cbSize);
    if (result == FALSE) {
        spdlog::error(L"isElevated - GetTokenInformation failed: {}", GetLastError());
        return std::nullopt;
    }

    return Elevation.TokenIsElevated;
}

static int WSMessageBox(const QString &title, const QString &text)
{
    QScopedPointer<QMessageBox> box(new QMessageBox(QMessageBox::Critical, title, text));
    return box->exec();
}

static void loadSystemLanguage(QTranslator &translator, QApplication *app)
{
    const QString language = LanguagesUtil::systemLanguage();
    const QString filename = ":/translations/windscribe_installer_" + language + ".qm";

    if (translator.load(filename)) {
        app->installTranslator(&translator);
    }
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

void writeLogFile(const std::wstring& installPath, const std::list<std::string> &logEntries)
{
    DWORD attrs = ::GetFileAttributes(installPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        spdlog::error(L"Log::writeFile - GetFileAttributes({}) failed ({})", installPath, ::GetLastError());
        return;
    }

    // This will be true if a symbolic link has been created on the folder, or a file within the folder.
    if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) {
        spdlog::error(L"Log::writeFile - the target folder is, or contains, a suspicious symbolic link ({})", installPath);
        return;
    }

    std::wstring fileName = installPath + L"\\installer.log";

    // The log file should not exist at this point. Fail if it does.
    FILE* fileHandle = nullptr;
    errno_t result = _wfopen_s(&fileHandle, fileName.c_str(), L"wxb,ccs=UTF-8");
    if ((result != 0) || (fileHandle == nullptr)) {
        spdlog::error(L"Log::writeFile - could not open {} ({})", fileName, result);
        return;
    }

    for (const auto &entry : logEntries) {
        fwrite(entry.c_str(), entry.size(), 1, fileHandle);
    }

    fflush(fileHandle);
    fclose(fileHandle);
}


#if !defined(_M_ARM64)
static bool isWindowsOnArm()
{
    // Notes:
    // Tried using GetSystemInfo and GetNativeSystemInfo.  Both return PROCESSOR_ARCHITECTURE_AMD64
    // when run on Windows 11 ARM64.
    // Dynamically loading the function to allow the installer to run on Windows 10 versions lacking the function export.
    HMODULE hDLL = ::GetModuleHandleA("kernel32.dll");
    if (hDLL == NULL) {
        spdlog::error(L"Failed to load the kernel32 module ({})", ::GetLastError());
        return false;
    }

    typedef BOOL (WINAPI* IsWow64Process2Func)(_In_ HANDLE hProcess, _Out_ USHORT* pProcessMachine, _Out_opt_ USHORT* pNativeMachine);

    IsWow64Process2Func isWow64Process2Func = (IsWow64Process2Func)::GetProcAddress(hDLL, "IsWow64Process2");
    if (isWow64Process2Func == NULL) {
        spdlog::error(L"Failed to load IsWow64Process2 function ({})", ::GetLastError());
        return false;
    }

    USHORT process_machine, native_machine;
    BOOL result = isWow64Process2Func(::GetCurrentProcess(), &process_machine, &native_machine);
    return (result && native_machine == IMAGE_FILE_MACHINE_ARM64);
}
#endif

}  // namespace


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    InstallerOptions ops;

    int argc = 0;
    QApplication a(argc, nullptr);
    QTranslator translator;

    loadSystemLanguage(translator, &a);

    // Initialize logger
    auto formatter = log_utils::createJsonFormatter();
    std::list<std::string> logEntries;
    auto logCallback = [&logEntries, &formatter](const spdlog::details::log_msg &msg) {
        spdlog::memory_buf_t dest;
        formatter->format(msg, dest);
        std::string str(dest.data(), dest.size());
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t >> converter;
        std::wstring wstr = converter.from_bytes(str);
        ::OutputDebugString(wstr.c_str());
        logEntries.push_back(str);
    };

    auto logger = spdlog::callback_logger_mt("installer", logCallback);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);

#if !defined(_M_ARM64)
    if (isWindowsOnArm()) {
        WSMessageBox(QObject::tr("Windscribe Installer"),
                     QObject::tr("This version of the Windscribe app will not operate correctly on your PC."
                                 "  Please download the 'ARM64' version from the Windscribe website to ensure"
                                 " optimal compatibility and performance."));
        return 0;
    }
#endif

    if (CheckCommandLineArgument(L"-help"))
    {
        WSMessageBox(QObject::tr("Windscribe Install Options"),
                     QString("%1\n\n-help\n%2\n\n-no-auto-start\n%3\n\n-silent\n%4\n\n-factory-reset\n%5\n\n-dir \"C:\\dirname\"\n%6\n\n-username \"my username\"\n%7\n\n-password \"my password\"\n%8")
                         .arg(QObject::tr("The Windscribe installer accepts the following optional commmand-line parameters: "))
                         .arg(QObject::tr("Show this information."))
                         .arg(QObject::tr("Do not launch the application after installation."))
                         .arg(QObject::tr("Instructs the installer to hide its user interface."))
                         .arg(QObject::tr("Delete existing preferences, logs, and other data, if they exist."))
                         .arg(QObject::tr("Overrides the default installation directory. Installation directory must be on the system drive."))
                         .arg(QObject::tr("Sets the username the application will use to automatically log in when first launched."))
                         .arg(QObject::tr("Sets the password the application will use to automatically log in when first launched.")));
        return 0;
    }

    // Useful for debugging
    // AllocConsole();
    // FILE *stream;
    // freopen_s(&stream, "CONOUT$", "w+t", stdout);

    ops.updating = CheckCommandLineArgument(L"-update") || CheckCommandLineArgument(L"-q");
    int expectedArgumentCount = ops.updating ? 2 : 1;

    int center_coord_index = 0;
    if (GetCommandLineArgumentIndex(L"-center", &center_coord_index)) {
            ops.centerX = _wtoi(argList[center_coord_index]);
            ops.centerY = _wtoi(argList[center_coord_index+1]);
        expectedArgumentCount += 3;
    }

    if (!ops.updating)
    {
        ops.silent = CheckCommandLineArgument(L"-silent");
        if (ops.silent) {
            expectedArgumentCount += 1;
        }
        ops.autostart = !CheckCommandLineArgument(L"-no-auto-start");
        if (!ops.autostart) {
            expectedArgumentCount += 1;
        }
        ops.factoryReset = CheckCommandLineArgument(L"-factory-reset");
        if (ops.factoryReset) {
            expectedArgumentCount += 1;
        }

        int install_path_index = 0;
        if (GetCommandLineArgumentIndex(L"-dir", &install_path_index)) {
            if (install_path_index > 0) {
                if (install_path_index >= argCount) {
                    WSMessageBox(QObject::tr("Windscribe Install Error"),
                                 QObject::tr("The -dir parameter was specified but the directory path was not."));
                    return 0;
                }

                ops.installPath = QString::fromStdWString(argList[install_path_index]);
                std::replace(ops.installPath.begin(), ops.installPath.end(), L'/', L'\\');
                expectedArgumentCount += 2;
            }
        }

        int username_index = 0;
        if (GetCommandLineArgumentIndex(L"-username", &username_index)) {
            if (username_index > 0) {
                if (username_index >= argCount) {
                    WSMessageBox(QObject::tr("Windscribe Install Error"),
                                 QObject::tr("The -username parameter was specified but the username was not."));
                    return 0;
                }

                ops.username = QString::fromStdWString(argList[username_index]);
                expectedArgumentCount += 2;
            }
        }

        int password_index = 0;
        if (GetCommandLineArgumentIndex(L"-password", &password_index)) {
            if (password_index > 0) {
                if (password_index >= argCount) {
                    WSMessageBox(QObject::tr("Windscribe Install Error"),
                                 QObject::tr("The -password parameter was specified but the password was not."));
                    return 0;
                }

                ops.password = QString::fromStdWString(argList[password_index]);
                expectedArgumentCount += 2;
            }
        }
    }

    if (argCount != expectedArgumentCount) {
        WSMessageBox(QObject::tr("Windscribe Install Error"),
                     QString("%1\n\n%2")
                         .arg(QObject::tr("Incorrect number of arguments passed to installer."))
                         .arg(QObject::tr("Use the -help argument to see available arguments and their format.")));
        return 0;
    }

    if (!ops.installPath.isEmpty() && !Path::isOnSystemDrive(ops.installPath.toStdWString())) {
        WSMessageBox(QObject::tr("Windscribe Install Error"),
                     QObject::tr("The specified installation path is not on the system drive.  To ensure the security of the application, and your system, it must be installed on the same drive as Windows."));
        return 0;
    }

    if (!ops.username.isEmpty()) {
        if (ops.password.isEmpty()) {
            WSMessageBox(QObject::tr("Windscribe Install Error"),
                         QString("%1\n\n%2")
                             .arg(QObject::tr("A username was specified but its corresponding password was not provided."))
                             .arg(QObject::tr("Use the -help argument to see available arguments and their format.")));
            return 0;
        }

        if (ops.username.indexOf('@') != -1) {
            WSMessageBox(QObject::tr("Windscribe Install Error"),
                         QObject::tr("Your username should not be an email address. Please try again."));
            return 0;
        }
    }

    if (!ops.password.isEmpty() && ops.username.isEmpty()) {
        WSMessageBox(QObject::tr("Windscribe Install Error"),
                     QString("%1\n\n%2")
                         .arg(QObject::tr("A password was specified but its corresponding username was not provided."))
                         .arg(QObject::tr("Use the -help argument to see available arguments and their format.")));
        return 0;
    }

    spdlog::info(L"Installing Windscribe version {}", ApplicationInfo::version());
    std::wstring exeName = argList[0];
    exeName = exeName.substr(exeName.find_last_of(L"/\\") + 1);
    spdlog::info(L"Command-line args: {}", sanitizedCommandLine(exeName, ops.username.toStdWString(), ops.password.toStdWString()));

    auto isAdmin = isElevated();
    if (!isAdmin.has_value()) {
        WSMessageBox(QObject::tr("Windscribe Install Error"),
                     QObject::tr("The installer was unable to determine if it is running with administrator rights.  Please report this failure to Windscribe support."));
        return 0;
    }

    // MainWindow does not create a UI when running in silent mode.  We'll have to use a standard messagebox
    // for this edge case.
    if (!isAdmin.value() && ops.silent) {
        WSMessageBox(QObject::tr("Windscribe Install Error"),
                     QObject::tr("You don't have sufficient permissions to run this application. Administrative privileges are required to install Windscribe."));
        return 0;
    }

    MainWindow w(isAdmin.value(), ops);
    w.show();

    int ret = a.exec();
    logger->flush();
    writeLogFile(Settings::instance().getPath(), logEntries);
    return ret;
}
