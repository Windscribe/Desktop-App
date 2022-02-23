#include "all_headers.h"
#include "logger.h"

void Logger::out(const wchar_t *format, ...)
{
    EnterCriticalSection(&cs_);
    if (file_)
    {
        va_list args;
        va_start (args, format);
        wchar_t buffer2[10000];
        _vsnwprintf (buffer2, 10000, format, args);

        std::wstring strTime = time_helper_.getCurrentTimeString<wchar_t>();
        std::wstring str = L"[" + strTime + L"] [service]\t " + std::wstring(buffer2) + L"\n";
#ifdef _DEBUG
		wprintf(L"%s", str.c_str());
#else
        fputws(str.c_str(), file_);
#endif
        fflush(file_);
        va_end (args);
    }
    LeaveCriticalSection(&cs_);
}

void Logger::out(const char *format, ...)
{
	EnterCriticalSection(&cs_);
	if (file_)
	{
		va_list args;
		va_start(args, format);
		char buffer2[10000];
		_vsnprintf(buffer2, 10000, format, args);

		std::string strTime = time_helper_.getCurrentTimeString<char>();
		std::string str = "[" + strTime + "] [service]\t " + std::string(buffer2) + "\n";
#ifdef _DEBUG
		printf("%s", str.c_str());
#else
		fputs(str.c_str(), file_);
#endif
		fflush(file_);
		va_end(args);
	}
	LeaveCriticalSection(&cs_);
}

Logger::Logger()
{
    InitializeCriticalSection(&cs_);
    wchar_t buffer[MAX_PATH];
    GetModuleFileName( NULL, buffer, MAX_PATH );
    std::wstring::size_type pos = std::wstring( buffer ).find_last_of(L"\\/");
    std::wstring path = std::wstring( buffer ).substr( 0, pos);

    std::wstring filePath = path + L"/windscribeservice.log";
    std::wstring prevFilePath = path + L"/windscribeservice_prev.log";

    if (isFileExist(filePath))
    {
        CopyFile(filePath.c_str(), prevFilePath.c_str(), FALSE);
    }

    file_ = _wfopen(filePath.c_str(), L"w+");
}

Logger::~Logger()
{
    if (file_)
    {
        fclose(file_);
    }
    DeleteCriticalSection(&cs_);
}

bool Logger::isFileExist(const std::wstring &fileName)
{
    std::wifstream infile(fileName);
    return infile.good();
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
