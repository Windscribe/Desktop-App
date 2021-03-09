#include "logger.h"

using namespace std;
Log::Log() : console(false), file_(NULL)
{
#ifdef _WIN32
    
 #else

 char path[PATH_MAX];
 ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
 path[len]   = 0;

 string path1 = path;
 unsigned int pos = path1.find("windscribe_installer");

 if(pos !=  string::npos)
   {
    Log::SetCurDir(L"/var/log/windscribe_installer");
   }

 #endif // _WIN32

}


Log::~Log()
{
	clearTempStrings();
	if (file_)
	{
		fclose(file_);
		file_ = NULL;
	}
}

void Log::clearTempStrings()
{
	for (auto it = tempStrings_.begin(); it != tempStrings_.end(); ++it)
	{
		delete (*it);
	}
	tempStrings_.clear();
}

void Log::setFilePath(const std::wstring &filePath)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	file_ = _wfopen(filePath.c_str(), L"w+");
	for (auto it = tempStrings_.begin(); it != tempStrings_.end(); ++it)
	{
		if ((*it)->isUnicode())
		{
			out((*it)->getWstring().c_str());
		}
		else
		{
			out((*it)->getString().c_str());
		}
	}
	clearTempStrings();
}


void Log::out(const wchar_t *format, ...)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = gmtime(&rawtime);
	wchar_t buffer[256];
	wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%d-%m %I:%M:%S", timeinfo);

	va_list args;
	va_start(args, format);
	wchar_t buffer2[10000];
	_vsnwprintf(buffer2, 10000, format, args);

	std::wstring strTime(buffer);
	std::wstring str = L"[" + strTime + L"]\t" + std::wstring(buffer2) + L"\n";

	if (file_)
	{
		fputws(str.c_str(), file_);
		fflush(file_);
	}
	else
	{
		tempStrings_.push_back(new LogString(buffer2));
	}
	va_end(args);
}

void Log::out(const std::wstring &str)
{
	out(str.c_str());
}

void Log::out(const char *format, ...)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);

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
	if (file_)
	{
		fputs(str.c_str(), file_);
		fflush(file_);
	}
	else
	{
		tempStrings_.push_back(new LogString(buffer2));
	}
	va_end(args);
}
