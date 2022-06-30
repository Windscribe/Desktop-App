#pragma once
#include <windows.h>
#include <string>

namespace CopyAndRun
{
	int runFirstPhase(const std::wstring &uninstExeFile, LPSTR lpszCmdParam);
};

