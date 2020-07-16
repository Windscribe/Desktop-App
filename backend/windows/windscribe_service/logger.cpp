#include "all_headers.h"
#include "logger.h"

void Logger::out(const wchar_t *format, ...)
{
    EnterCriticalSection(&cs_);
    if (file_)
    {
        time_t rawtime;
        time(&rawtime);
        struct tm *timeinfo = gmtime(&rawtime);
        wchar_t buffer[256];
        wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%d-%m %I:%M:%S", timeinfo);

        va_list args;
        va_start (args, format);
        wchar_t buffer2[10000];
        _vsnwprintf (buffer2, 10000, format, args);

        std::wstring strTime(buffer);
        std::wstring str = L"[" + strTime + L"]\t" + std::wstring(buffer2) + L"\n";
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
		time_t rawtime;
		time(&rawtime);
		struct tm *timeinfo = gmtime(&rawtime);
		char buffer[256];
		strftime(buffer, sizeof(buffer), "%d-%m %I:%M:%S", timeinfo);

		va_list args;
		va_start(args, format);
		char buffer2[10000];
		_vsnprintf(buffer2, 10000, format, args);

		std::string strTime(buffer);
		std::string str = "[" + strTime + "]\t" + std::string(buffer2) + "\n";
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
