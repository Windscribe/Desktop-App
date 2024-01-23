#include "logger.h"

#include <Windows.h>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

Log::Log()
{
    start_ = std::chrono::system_clock::now();
}

Log::~Log()
{
}

void Log::init(bool installing)
{
    installing_ = installing;
}

void Log::out(const char* format, ...)
{
    // Using dynamic allocation here for the string buffers as the VS2019 compiler was warning
    // about stack overflow potential.
    va_list args;
    va_start(args, format);
    unique_ptr<char[]> logMsg(new char[10000]);
    _vsnprintf_s(logMsg.get(), 10000, _TRUNCATE, format, args);
    va_end(args);

    wostringstream stream;
    stream << logMsg.get();

    out(stream.str());
}

void Log::out(const wchar_t* format, ...)
{
    // Using dynamic allocation here for the string buffers as the VS2019 compiler was warning
    // about stack overflow potential.
    va_list args;
    va_start(args, format);
    unique_ptr<wchar_t[]> logMsg(new wchar_t[10000]);
    _vsnwprintf_s(logMsg.get(), 10000, _TRUNCATE, format, args);
    va_end(args);

    out(wstring(logMsg.get()));
}

void Log::out(const wstring& message)
{
    const auto now = std::chrono::system_clock::now();
    const auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* utc_tm = std::gmtime(&now_c);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) % 1000;
    const int millisecondsSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();


    wostringstream stream;
    stream << L"[" << std::put_time(utc_tm, L"%d%m%y %H:%M:%S")
           << L':' << std::setfill(L'0') << std::setw(3) << milliseconds.count()
           << L"      " << static_cast<int>(millisecondsSinceStart / 1000) << "."
           << std::setfill(L'0') << std::setw(3) << (millisecondsSinceStart % 1000)
           << L"]\t" << message << endl;

    if (installing_) {
        lock_guard<recursive_mutex> lock(mutex_);
        logEntries_.push_back(stream.str());
    }
    else {
        // The uninstaller logs to the system debugger, so we do not leave an uninstaller log
        // (cruft) on the user's device.
        ::OutputDebugStringW(stream.str().c_str());
    }
}

void
Log::WSDebugMessage(const wchar_t* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    wchar_t szMsg[1024];
    _vsnwprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    // Send the debug string to the debugger.
    ::OutputDebugString(szMsg);
}

void Log::writeFile(const wstring& installPath) const
{
    if (!installing_) {
        return;
    }

    DWORD attrs = ::GetFileAttributes(installPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        writeToSystemDebugger();
        WSDebugMessage(L"Log::writeFile - GetFileAttributes(%s) failed (%lu)", installPath.c_str(), ::GetLastError());
        return;
    }

    // This will be true if a symbolic link has been created on the folder, or a file within the folder.
    if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) {
        writeToSystemDebugger();
        WSDebugMessage(L"Log::writeFile - the target folder is, or contains, a suspicious symbolic link (%s)", installPath.c_str());
        return;
    }

    wstring fileName = installPath + L"\\log_installer.txt";

    // The log file should not exist at this point. Fail if it does.
    FILE* fileHandle = nullptr;
    errno_t result = _wfopen_s(&fileHandle, fileName.c_str(), L"wx");
    if ((result != 0) || (fileHandle == nullptr)) {
        writeToSystemDebugger();
        WSDebugMessage(L"Log::writeFile - could not open %s (%d)", fileName.c_str(), result);
        return;
    }

    for (const auto &entry : logEntries_) {
        fputws(entry.c_str(), fileHandle);
    }

    fflush(fileHandle);
    fclose(fileHandle);
}

void Log::writeToSystemDebugger() const
{
    for (const auto &entry : logEntries_) {
        ::OutputDebugStringW(entry.c_str());
    }
}
