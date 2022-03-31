#include "arguments_parser.h"
#include <windows.h>
#include <shellapi.h>
#include <cwctype>

namespace {

	bool compareChar(const wchar_t & c1, const wchar_t & c2)
	{
		if (c1 == c2)
			return true;
		else if (std::towupper(c1) == std::towupper(c2))
			return true;
		return false;
	}

	bool caseInsensitiveStringCompare(const std::wstring & str1, const std::wstring &str2)
	{
		return ((str1.size() == str2.size()) &&
			std::equal(str1.begin(), str1.end(), str2.begin(), &compareChar));
	}
}

bool ArgumentsParser::parse()
{
	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (szArglist == nullptr)
	{
		return false;
	}

	for (int i = 0; i < nArgs; i++)
	{
		if (i == 0)
		{
			executablePath_ = szArglist[i];
			continue;
		}

		std::wstring curStr = szArglist[i];
		if ((curStr != L"") && (curStr[0] == '/'))
		{
			unsigned int p = curStr.find(L"=");

			if (p != std::wstring::npos)
			{
				std::wstring AName = curStr.substr(0, p);
				std::wstring AValue = curStr.substr(p + 1);
				args_.push_back(std::make_pair(AName, AValue));
			}
			else
			{
				args_.push_back(std::make_pair(curStr, L""));
			}
		}
	}

	LocalFree(szArglist);
	return true;
}


std::wstring ArgumentsParser::getExecutablePath() const
{
	return executablePath_;
}

bool ArgumentsParser::getArg(const std::wstring &argName, std::wstring *outValue)
{
	for (auto it : args_)
	{
		if (caseInsensitiveStringCompare(it.first, argName))
		{
			if (outValue)
			{
				*outValue = it.second;
			}
			return true;
		}
	}
	return false;
}