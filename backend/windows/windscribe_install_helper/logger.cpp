#include "logger.h"

#include <Windows.h>

#include <cstdarg>

using namespace std;

static void WSDebugMessage(const wchar_t* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    wchar_t szMsg[1024];
    _vsnwprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    ::OutputDebugString(szMsg);
}

Logger::Logger(const wchar_t *path)
{
    errno_t result = _wfopen_s(&file_, path, L"wt,ccs=UTF-8");
    if ((result != 0) || (file_ == nullptr)) {
        WSDebugMessage(L"Windscribe install helper failed to open its log file '%s' (%d)", path, result);
    }
}

Logger::~Logger()
{
    if (file_) {
        fclose(file_);
    }
}

void Logger::outCmdLine(int argc, wchar_t* argv[])
{
    if (file_) {
        fputws(L"---Command line parameters\n", file_);
        for (int i = 0; i < argc; ++i) {
            fputws(argv[i], file_);
            fputws(L"\n", file_);
        }
        fputws(L"--------------------------\n", file_);
    }
}

void Logger::outStr(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    wchar_t szMsg[1024];
    _vsnwprintf_s(szMsg, 1024, _TRUNCATE, format, args);
    va_end(args);

    if (file_) {
        fputws(szMsg, file_);
    }
    else {
        ::OutputDebugString(szMsg);
    }
}
