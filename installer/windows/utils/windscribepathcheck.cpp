#include "windscribepathcheck.h"
#include <cwctype>
#include <shlwapi.h>

namespace
{
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

	bool isPathsEqual(const std::wstring &path1, const std::wstring &path2)
	{
		if (path1.size() < MAX_PATH && path2.size() < MAX_PATH)
		{
			TCHAR buf1[MAX_PATH];
			wcscpy_s(buf1, path1.c_str());
			PathRemoveBackslash(buf1);

			TCHAR buf2[MAX_PATH];
			wcscpy_s(buf2, path2.c_str());
			PathRemoveBackslash(buf2);

			return caseInsensitiveStringCompare(buf1, buf2);
		}
		else
		{
			return false;
		}
	}
}

namespace WindscribePathCheck
{
	bool isNeedAppendWindscribeSubdirectory(const std::wstring &installPath, const std::wstring &prevInstallPath)
	{
		// this is current Windscribe dir, then nothing to append
		if (!installPath.empty() && !prevInstallPath.empty() && isPathsEqual(installPath, prevInstallPath))
		{
			return false;
		}

		// is directory exists?
		bool isDirExists = false;
		DWORD ftyp = GetFileAttributes(installPath.c_str());
		if (ftyp != INVALID_FILE_ATTRIBUTES)
		{
			if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
				isDirExists = true;
		}
		// check if directory empty
		bool isDirEmpty = false;
		if (isDirExists)
		{
			isDirEmpty = PathIsDirectoryEmpty(installPath.c_str());
		}

		//  if the selected directory has files in it, need to append \Windscribe to it
		if (isDirExists && !isDirEmpty)
		{
			return true;
		}

		return false;
	}

	std::wstring appendToDirectory(const std::wstring &dir, const std::wstring &suffix)
	{
		if (dir.size() < MAX_PATH)
		{
			TCHAR buf[MAX_PATH];
			wcscpy_s(buf, dir.c_str());
			if (PathAppend(buf, suffix.c_str()))
			{
				return buf;
			}
		}
		return L"";
	}
}