//---------------------------------------------------------------------------
#ifndef loggerH
#define loggerH

#include <string>

#include <mutex>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <fstream>

//---------------------------------------------------------------------------

class Log
{
public:
	static Log &instance()
	{
		static Log log;
		return log;
	}


	void out(const wchar_t *format, ...);
	void out(const char *format, ...);
	void out(const std::wstring &str);


	void setFilePath(const std::wstring &filePath);

private:
	bool console;
	std::recursive_mutex mutex;
	FILE *file_;

	struct LogString
	{
		LogString(const std::string &s)
		{
			isUnicode_ = false;
			p = new std::string(s);
		}

		LogString(const std::wstring &s)
		{
			isUnicode_ = true;
			p = new std::wstring(s);
		}

		~LogString()
		{
			if (isUnicode_)
			{
				delete ((std::wstring *)(p));
			}
			else
			{
				delete ((std::string *)(p));
			}
		}

		bool isUnicode() const { return isUnicode_; }
		std::wstring getWstring() { return *((std::wstring *)(p)); }
		std::string getString() { return *((std::string *)(p)); }

	private:
		bool isUnicode_;
		void *p;    // is isUnicode == true, then p pointer to wstring, else to string
	};

	std::vector<LogString *> tempStrings_;
	
	void clearTempStrings();
	Log();
	~Log();
};


#endif
