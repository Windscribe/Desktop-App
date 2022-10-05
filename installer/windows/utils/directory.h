#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <Windows.h>
#include <string>

class Directory
{
public:
   static std::wstring ReplaceSystemDirWithSysWow64(const std::wstring Path);
   static bool DirExists(const std::wstring Name);
   static DWORD InternalGetFileAttr(const std::wstring Name);
   static std::wstring GetSysWow64Dir();
   static std::wstring GetSystemDir();
   static bool caseInsensitiveStringCompare(const std::wstring& str1, const std::wstring& str2);
   static bool CreateFullPath(const std::wstring &s);
};

#endif // DIRECTORY_H
