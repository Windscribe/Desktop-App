#pragma once
#include <string>

namespace WindscribePathCheck
{
	bool isNeedAppendWindscribeSubdirectory(const std::wstring &installPath, const std::wstring &prevInstallPath);
	std::wstring appendToDirectory(const std::wstring &dir, const std::wstring &suffix);
	bool in32BitProgramFilesFolder(const std::wstring path);
}
