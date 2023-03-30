#include "directory.h"
#include "path.h"

using namespace std;

wstring Directory::GetSysWow64Dir()
//{ Returns fully qualified path of the SysWow64 directory on 64-bit Windows.
//  Returns '' if there is no SysWow64 directory (e.g. running 32-bit Windows). }
{
    UINT Res;
    wchar_t Buf[MAX_PATH];

    wstring Result = L"";

    //  { Note: This function does exist on 32-bit XP, but always returns 0 }

    Res = GetSystemWow64Directory(Buf, sizeof(Buf) / sizeof(Buf[0]));
    if ((Res > 0) && (Res < sizeof(Buf) / sizeof(Buf[0])))
    {
        Result = Buf;
    }

    return Result;
}

wstring Directory::GetSystemDir()
{
    //{ Returns fully qualified path of the Windows System directory. Only includes a
    //  trailing backslash if the Windows System directory is the root directory. }

    wchar_t Buf[MAX_PATH];

    GetSystemDirectory(Buf, sizeof(Buf) / sizeof(Buf[0]));
    return Buf;
}

bool Directory::caseInsensitiveStringCompare(const wstring& str1, const wstring& str2)
{
    if (str1.size() != str2.size())
    {
        return false;
    }

    for (wstring::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2)
    {
        if (tolower(*c1) != tolower(*c2))
        {
            return false;
        }
    }

    return true;
}

wstring Directory::ReplaceSystemDirWithSysWow64(const wstring Path)
{
    //{ If the user is running 64-bit Windows and Path begins with
    //  'x:\windows\system32' it replaces it with 'x:\windows\syswow64', like the
    //  file system redirector would do. Otherwise, Path is returned unchanged. }

    wstring SysWow64Dir, SysDir;
    size_t L;

    SysWow64Dir = GetSysWow64Dir();
    if (SysWow64Dir != L"")
    {
        SysDir = GetSystemDir();
        //    { x:\windows\system32 -> x:\windows\syswow64
        //      x:\windows\system32\ -> x:\windows\syswow64\
        //      x:\windows\system32\filename -> x:\windows\syswow64\filename
        //      x:\windows\system32x -> x:\windows\syswow64x  <- yes, like Windows! }
        L = SysDir.length();
        if ((Path.length() == L) ||
            ((Path.length() > L)/* && (path.PathCharIsTrailByte(Path, L)==false)*/))
        {
            // { ^ avoid splitting a double-byte character }
            wstring str = wstring(Path.c_str(), L);
            if (caseInsensitiveStringCompare(str.c_str(), SysDir) == true)
            {
                return SysWow64Dir + Path.substr(L);
            }
        }
    }

    return Path;
}


DWORD Directory::InternalGetFileAttr(const wstring Name)
{
    return GetFileAttributes(Path::RemoveBackslashUnlessRoot(Name).c_str());
}

bool Directory::DirExists(const wstring Name)
{
    //{ Returns True if the specified directory name exists. The specified name
    //  may include a trailing backslash.
    //  NOTE: Delphi's FileCtrl unit has a similar function called DirectoryExists.
    //  However, the implementation is different between Delphi 1 and 2. (Delphi 1
    //  does not count hidden or system directories as existing.) }

    DWORD Attr = InternalGetFileAttr(Name);
    return (Attr != INVALID_FILE_ATTRIBUTES) && (Attr & FILE_ATTRIBUTE_DIRECTORY);

}

bool Directory::CreateFullPath(const wstring& s)
{
    if (s.length() <= 1)
        return true;

    auto s_copy{ s };
    const wstring::size_type s_len = s_copy.length();
    for (wstring::size_type i = 1; i <= s_len; ++i) {
        if (s_copy[i] == L'/' || s_copy[i] == L'\\' || i == s_len) {
            // Create a directory, but don't attempt to create a disk.
            if (s_copy[i - 1] == L'/' || s_copy[i - 1] == L'\\' || s_copy[i - 1] == ':')
                continue;
            const auto old_char = s_copy[i];
            s_copy[i] = 0;
            const int create_result = _wmkdir(s_copy.c_str());
            s_copy[i] = old_char;
            if (create_result == -1 && errno != EEXIST)
                return false;
        }
    }
    return true;
}
