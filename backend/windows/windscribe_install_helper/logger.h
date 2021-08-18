#pragma once

#include <stdio.h>

class Logger
{
public:
	explicit Logger(const wchar_t *path);
	virtual ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

	void outStr(const wchar_t *str);
	void outStr(const char *str);
	void outCmdLine(int argc, wchar_t* argv[]);

private:
	FILE *file_;
};

