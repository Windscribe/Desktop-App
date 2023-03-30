#include "logger.h"

#include <sstream>

#include "utils.h"

Logger::Logger()
{
    ::InitializeCriticalSection(&cs_);

    wchar_t buffer[MAX_PATH];
    ::GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    std::wstring path = std::wstring(buffer).substr(0, pos);

    std::wstring filePath = path + L"\\windscribeservice.log";
    std::wstring prevFilePath = path + L"\\windscribeservice_prev.log";

    if (Utils::isFileExists(filePath.c_str())) {
        ::CopyFile(filePath.c_str(), prevFilePath.c_str(), FALSE);
    }

    // Cannot use _wfopen_s here, as it will lock the file for exclusive access
    // until the helper is stopped.
    file_ = _wfsopen(filePath.c_str(), L"w", _SH_DENYWR);
    if (file_ == nullptr) {
        debugOut("Logger could not open: %ls", filePath.c_str());
    }
}

Logger::~Logger()
{
    if (file_) {
        fclose(file_);
    }

    ::DeleteCriticalSection(&cs_);
}

void Logger::out(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    wchar_t buffer[4096];
    _vsnwprintf_s(buffer, 4096, _TRUNCATE, format, args);
    va_end(args);

    out(std::wstring(buffer));
}

void Logger::out(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[4096];
    _vsnprintf_s(buffer, 4096, _TRUNCATE, format, args);
    va_end(args);

    std::wostringstream stream;
    stream << buffer;

    out(stream.str());
}

void Logger::out(const std::wstring& message)
{
    std::wostringstream stream;
    stream << L"[" << time_helper_.getCurrentTimeString<wchar_t>() << L"] [service]\t " << message << std::endl;

    ::EnterCriticalSection(&cs_);
    if (file_)
    {
        #ifdef _DEBUG
        wprintf(L"%s", stream.str().c_str());
        #else
        fputws(stream.str().c_str(), file_);
        fflush(file_);
        #endif
    }
    else {
        ::OutputDebugStringW(stream.str().c_str());
    }
    ::LeaveCriticalSection(&cs_);
}

void Logger::debugOut(const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    char szMsg[1024];
    _vsnprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    ::OutputDebugStringA(szMsg);
}

void Logger::debugOut(const char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    char szMsg[1024];
    szMsg[1023] = '\0';

    _vsnprintf(szMsg, 1023, format, arg_list);
    va_end(arg_list);

    ::OutputDebugStringA(szMsg);
}
