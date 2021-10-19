//---------------------------------------------------------------------------
#ifndef loggerH
#define loggerH

#include <string>
#include <mutex>

#include <Windows.h>
#include <tchar.h>

//---------------------------------------------------------------------------
class Log
{
public:
	static Log &instance()
	{
		static Log log;
		return log;
	}

    void init(bool installing);
    void out(const char *format, ...);
    void out(const wchar_t *format, ...);
    void out(const std::wstring &str);

    static void WSDebugMessage(const TCHAR* format, ...);

private:
    std::recursive_mutex mutex;
	FILE *file_;

    Log();
	~Log();
};

#endif
