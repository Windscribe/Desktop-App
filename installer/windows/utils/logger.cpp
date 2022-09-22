#include "logger.h"

#include <shlobj.h>

#include <fstream>
#include <sstream>

#include "utils.h"

using namespace std;

Log::Log()
{
}

Log::~Log()
{
    if (file_)
    {
        fclose(file_);
        file_ = NULL;
    }
}

void Log::init(bool installing, const wstring& installPath)
{
    // The uninstaller logs to the system debugger, so we do not leave an uninstaller log
    // (cruft) on the user's device.
    if (!installing) {
        return;
    }

    // Make sure the logging folder for the installer exists so we can create the log file.
    if (::SHCreateDirectoryEx(NULL, installPath.c_str(), NULL) != ERROR_SUCCESS)
    {
        if (::GetLastError() != ERROR_ALREADY_EXISTS)
        {
            WSDebugMessage(_T("Logger could not create: %ls"), installPath.c_str());
            return;
        }
    }

    wstring fileName = installPath + L"\\log_installer.txt";

    // The log file shouldn't exist, nuke it if it does.
    Utils::deleteFile(fileName);

    // Only open the log file for writing if it doesn't exist.  If it does still exist,
    // something truly odd is going on (PC hacked?), and we'll log to the system
    // debugger instead.
    file_ = _wfsopen(fileName.c_str(), L"wx", _SH_DENYWR);
    if (file_ == NULL) {
        WSDebugMessage(_T("Logger could not open: %ls"), fileName.c_str());
    }
}

void Log::out(const char* format, ...)
{
    // Using dynamic allocation here for the string buffers as the VS2019 compiler was warning
    // about stack overflow potential.

    time_t rawtime;
    time(&rawtime);
    struct tm* timeinfo = gmtime(&rawtime);
    char timeStr[256];
    strftime(timeStr, 256, "%d-%m %I:%M:%S", timeinfo);

    va_list args;
    va_start(args, format);
    unique_ptr<char[]> logMsg(new char[10000]);
    _vsnprintf_s(logMsg.get(), 10000, _TRUNCATE, format, args);
    va_end(args);

    ostringstream stream;
    stream << "[" << timeStr << "]\t" << logMsg.get() << endl;

    if (file_)
    {
        lock_guard<recursive_mutex> lock(mutex);
        fputs(stream.str().c_str(), file_);
        fflush(file_);
    }
    else {
        ::OutputDebugStringA(stream.str().c_str());
    }
}

void Log::out(const wchar_t* format, ...)
{
    // Using dynamic allocation here for the string buffers as the VS2019 compiler was warning
    // about stack overflow potential.

    time_t rawtime;
    time(&rawtime);
    struct tm* timeinfo = gmtime(&rawtime);
    wchar_t timeStr[256];
    wcsftime(timeStr, 256, L"%d-%m %I:%M:%S", timeinfo);

    va_list args;
    va_start(args, format);
    unique_ptr<wchar_t[]> logMsg(new wchar_t[10000]);
    _vsnwprintf_s(logMsg.get(), 10000, _TRUNCATE, format, args);
    va_end(args);

    wostringstream stream;
    stream << L"[" << timeStr << L"]\t" << logMsg.get() << endl;

    if (file_)
    {
        lock_guard<recursive_mutex> lock(mutex);
        fputws(stream.str().c_str(), file_);
        fflush(file_);
    }
    else {
        ::OutputDebugStringW(stream.str().c_str());
    }
}

void Log::out(const wstring& str)
{
    out(str.c_str());
}

void
Log::WSDebugMessage(const TCHAR* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    TCHAR szMsg[1024];
    _vsntprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    // Send the debug string to the debugger.
    ::OutputDebugString(szMsg);
}
