#ifndef PATH_H
#define PATH_H

#include <Windows.h>
#include <string>

class Path
{
public:
    static bool PathCharIsSlash(const char C);
	static bool PathCharIsSlash(const wchar_t C);
	static std::wstring AddBackslash(const std::wstring S);
	static std::wstring PathExpand(const std::wstring Filename);
	static std::wstring RemoveBackslashUnlessRoot(const std::wstring S);
	static unsigned int PathDrivePartLengthEx(const std::wstring Filename, const bool IncludeSignificantSlash);
	static std::wstring PathExtractName(const std::wstring Filename);
	static std::wstring PathExtractDir(const std::wstring Filename);
	static std::wstring PathExtractPath(const std::wstring Filename);
	static unsigned int PathPathPartLength(const std::wstring Filename, const bool IncludeSlashesAfterPath);
	static std::wstring PathExtractDrive(const std::wstring Filename);
	static unsigned int PathDrivePartLength(const std::wstring Filename);
	static std::wstring PathChangeExt(const std::wstring Filename, const std::wstring Extension);
	static unsigned int PathExtensionPos(const std::wstring Filename);
	static std::wstring PathExtractExt(const std::wstring &s);
};

#endif // PATH_H
