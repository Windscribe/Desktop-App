#include <shlobj.h>

#include <codecvt>
#include <fstream>

#include "logger.h"

using namespace std;


// Need to use the same logging folder as the Qt-based GUI and engine use.
static wstring
GetLoggingFolder(void)
{
    wstring folder;

    PWSTR pUnicodePath;
    HRESULT hResult = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pUnicodePath);

    if (SUCCEEDED(hResult))
    {
        folder = wstring(pUnicodePath);
        folder += wstring(L"\\Windscribe\\Windscribe2");
        ::CoTaskMemFree(pUnicodePath);
    }
    else {
        Log::WSDebugMessage(_T("GetLogsFolder failed: %d"), HRESULT_CODE(hResult));
    }

    return folder;
}


Log::Log() : file_(NULL)
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

void Log::init(bool installing)
{
    wstring folder = GetLoggingFolder();

    // Make sure the logging folder exists.
    if (::SHCreateDirectoryEx(NULL, folder.c_str(), NULL) != ERROR_SUCCESS)
    {
        if (::GetLastError() != ERROR_ALREADY_EXISTS)
        {
            WSDebugMessage(_T("Logger could not create: %ls"), folder.c_str());
            return;
        }
    }

    const wchar_t* openMode = L"w+";
    wstring filePath(folder);

    if (installing)
    {
        filePath += L"\\log_installer.txt";
        
        wstring prevFilePath(folder);
        prevFilePath += L"\\prev_log_installer.txt";

        wifstream infile(filePath);

        if (infile.good()) {
            ::CopyFile(filePath.c_str(), prevFilePath.c_str(), FALSE);
        }

        infile.close();
    }
    else
    {
        // Uninstall exe runs in multiple phases, so we'll append to the log file rather
        // than create a new one.
        const wchar_t* openMode = L"a+";
        filePath += L"\\log_uninstaller.txt";
    }

    file_ = _wfopen(filePath.c_str(), openMode);
    if (file_ == NULL) {
        WSDebugMessage(_T("Logger could not open: %ls"), filePath.c_str());
    }
}


void Log::out(const char *format, ...)
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
    va_end(args);

    string strTime(buffer);
    string str = "[" + strTime + "]\t" + string(buffer2) + "\n";

    std::lock_guard<std::recursive_mutex> lock(mutex);

    if (file_)
    {
        fputs(str.c_str(), file_);
        fflush(file_);
    }
    else
    {
        wstring debug = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(str);
        WSDebugMessage(debug.c_str());
    }
}

void Log::out(const wchar_t *format, ...)
{
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = gmtime(&rawtime);
    wchar_t buffer[256];
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%d-%m %I:%M:%S", timeinfo);

    va_list args;
    va_start(args, format);
    wchar_t buffer2[10000];
    _vsnwprintf(buffer2, 10000, format, args);
    va_end(args);

    std::wstring strTime(buffer);
    std::wstring str = L"[" + strTime + L"]\t" + std::wstring(buffer2) + L"\n";

    std::lock_guard<std::recursive_mutex> lock(mutex);

    if (file_)
    {
        fputws(str.c_str(), file_);
        fflush(file_);
    }
    else {
        WSDebugMessage(str.c_str());
    }
}

void Log::out(const std::wstring &str)
{
    out(str.c_str());
}

void
Log::WSDebugMessage(const TCHAR* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    TCHAR szMsg[1024];
    szMsg[1023] = _T('\0');

    _vsntprintf(szMsg, 1023, format, arg_list);
    va_end(arg_list);

    // Send the debug string to the debugger.
    ::OutputDebugString(szMsg);
}
