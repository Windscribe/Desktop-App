#include "Logger.h"


Logger::Logger(const wchar_t *path)
{
	file_ = _wfopen(path, L"w");
}


Logger::~Logger()
{
	if (file_)
	{
		fclose(file_);
	}
}

void Logger::outStr(const wchar_t *str)
{
	if (file_)
	{
		fputws(str, file_);
	}
}

void Logger::outStr(const char *str)
{
	if (file_)
	{
		fputs(str, file_);
	}
}

void Logger::outCmdLine(int argc, wchar_t* argv[])
{
	if (file_)
	{
		fputws(L"---Command line parameters\n", file_);
		for (int i = 0; i < argc; ++i)
		{
			fputws(argv[i], file_);
			fputws(L"\n", file_);
		}
		fputws(L"--------------------------\n", file_);
	}
}