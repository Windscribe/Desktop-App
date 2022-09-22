#include "all_headers.h"
#include "logger.h"
#include "utils.h"

void Logger::out(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    wchar_t buffer2[4096];
    _vsnwprintf_s(buffer2, 4096, _TRUNCATE, format, args);
    va_end(args);

    std::wstring strTime = time_helper_.getCurrentTimeString<wchar_t>();
    std::wstring str = L"[" + strTime + L"] [service]\t " + std::wstring(buffer2) + L"\n";

    ::EnterCriticalSection(&cs_);
    if (file_)
    {
#ifdef _DEBUG
        wprintf(L"%s", str.c_str());
#else
        fputws(str.c_str(), file_);
#endif
        fflush(file_);
    }
    else {
        ::OutputDebugStringW(str.c_str());
    }
    ::LeaveCriticalSection(&cs_);
}

void Logger::out(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer2[4096];
    _vsnprintf_s(buffer2, 4096, _TRUNCATE, format, args);
    va_end(args);

    std::string strTime = time_helper_.getCurrentTimeString<char>();
    std::string str = "[" + strTime + "] [service]\t " + std::string(buffer2) + "\n";

    ::EnterCriticalSection(&cs_);
    if (file_)
    {
#ifdef _DEBUG
        printf("%s", str.c_str());
#else
        fputs(str.c_str(), file_);
#endif
        fflush(file_);
    }
    else {
        ::OutputDebugStringA(str.c_str());
    }
    ::LeaveCriticalSection(&cs_);
}

Logger::Logger()
{
    ::InitializeCriticalSection(&cs_);

    wchar_t buffer[MAX_PATH];
    ::GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    std::wstring path = std::wstring(buffer).substr(0, pos);

    std::wstring filePath = path + L"/windscribeservice.log";
    std::wstring prevFilePath = path + L"/windscribeservice_prev.log";

    // This process runs with elevated privilege and its possible the user installed it in
    // a user-accessible folder.  DeleteFile() will remove any potentially dangerous symbolic
    // links created by an attacker (e.g. attacker symbolic links this log file to point at
    // a system file).

    if (Utils::isFileExists(filePath.c_str()))
    {
        ::DeleteFile(prevFilePath.c_str());
        ::CopyFile(filePath.c_str(), prevFilePath.c_str(), TRUE);
    }

    ::DeleteFile(filePath.c_str());

    file_ = _wfsopen(filePath.c_str(), L"wx", _SH_DENYWR);
    if (file_ == NULL) {
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

void Logger::debugOut(const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    char szMsg[1024];
    _vsnprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    ::OutputDebugStringA(szMsg);
}
