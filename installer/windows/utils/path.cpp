#include "path.h"

#include <shlwapi.h>

using namespace std;

wstring Path::PathExpand(const wstring Filename)
{
    DWORD Res;

    TCHAR** FilePart = { nullptr };
    wchar_t Buf[4096];

    wstring Result;

    DWORD nBufferLength = sizeof(Buf) / sizeof(Buf[0]);
    Res = GetFullPathName(Filename.c_str(), nBufferLength, Buf, FilePart);

    if ((Res > 0) && (Res < sizeof(Buf) / sizeof(Buf[0])))
    {
        Result = Buf;
    }
    else
    {
        Result = Filename;
    }

    return Result;
}

bool Path::PathCharIsSlash(const char C)
//{ Returns True if C is a backslash or slash. }
{
    bool Result = (C == '\\') || (C == '/');

    return Result;
}

bool Path::PathCharIsSlash(const wchar_t C)
//{ Returns True if C is a backslash or slash. }
{
    bool Result = (C == '\\') || (C == '/');

    return Result;
}

wstring Path::AddBackslash(const wstring S)
//{ Returns S plus a trailing backslash, unless S is an empty string or already
//  ends in a backslash/slash. }
{
    wstring Result;
    if ((S != L"") && (PathCharIsSlash(S[S.length() - 1]) == false))
    {
        Result = S + L"\\";
    }
    else
    {
        Result = S;
    }

    return Result;
}



wstring Path::RemoveBackslashUnlessRoot(const wstring S)
//{ Returns S minus any trailing slashes, unless S specifies the root directory
 // of a drive (i.e. 'C:\' or '\'), in which case it leaves 1 slash. }
{
    wstring Result;

    UINT DrivePartLen = PathDrivePartLengthEx(S, true);
    size_t I = S.length();

    while ((I > DrivePartLen) && PathCharIsSlash(S[I - 1]))
        I--;

    if (I == S.length())
        Result = S;
    else
        Result = S.substr(0, I + 1);

    return Result;
}


wstring Path::PathExtractName(const wstring Filename)
{
    //{ Returns the filename portion of Filename (e.g. 'filename.txt'). If Filename
    //  ends in a slash or consists only of a drive part, the result will be an empty
    //  string.
    //  This function is essentially the opposite of PathExtractPath. }
    unsigned int I;

    I = PathPathPartLength(Filename, true);

    return Filename.substr(I + 1);
}



wstring Path::PathExtractDir(const wstring Filename)
//{ Like PathExtractPath, but strips any trailing slashes, unless the resulting
//  path is the root directory of a drive (i.e. 'C:\' or '\'). }
{
    unsigned int I;

    I = PathPathPartLength(Filename, false);
    return  Filename.substr(0, I + 1);
}

/*
int Path::PathCharLength(const wstring S, const int Index)
{
//{ Returns the length in bytes of the character at Index in S.
//  Notes:
//  1. If Index specifies the last character in S, 1 will always be returned,
//     even if the last character is a lead byte.
//  2. If a lead byte is followed by a null character (e.g. #131#0), 2 will be
//     returned. This mimics the behavior of MultiByteToWideChar and CharPrev,
//     but not CharNext(P)-P, which would stop on the null. }
 int Result;

 #ifndef UNICODE
  if (IsDBCSLeadByte(S[Index]) && (Index < S.length()))
    Result = 2;
  else
 #endif
 Result = 1;

 return Result;
}
*/



unsigned int Path::PathDrivePartLengthEx(const wstring Filename, const bool IncludeSignificantSlash)
{
    //{ Returns length of the drive portion of Filename, or 0 if there is no drive
    //  portion.
    //  If IncludeSignificantSlash is True, the drive portion can include a trailing
    //  slash if it is significant to the meaning of the path (i.e. 'x:' and 'x:\'
    //  are not equivalent, nor are '\' and '').
    //  If IncludeSignificantSlash is False, the function works as follows:
    //    'x:file'              -> 2  ('x:')
    //    'x:\file'             -> 2  ('x:')
    //    '\\server\share\file' -> 14 ('\\server\share')
    //    '\file'               -> 0  ('')
    //  If IncludeSignificantSlash is True, the function works as follows:
    //    'x:file'              -> 2  ('x:')
    //    'x:\file'             -> 3  ('x:\')
    //    '\\server\share\file' -> 14 ('\\server\share')
    //    '\file'               -> 1  ('\')
    //  Note: This is MBCS-safe, unlike the Delphi's ExtractFileDrive function.
    //  (Computer and share names can include multi-byte characters!) }

    unsigned int I, C;

    unsigned int Result;
    size_t Len = Filename.length();

    // { \\server\share }
    if ((Len >= 2) && PathCharIsSlash(Filename[0]) && PathCharIsSlash(Filename[1]))
    {
        I = 3;
        C = 0;
        while (I <= Len)
        {
            if (PathCharIsSlash(Filename[I]) == true)
            {
                C++;
                if (C >= 2)
                    break;

                while (1)
                {
                    I++;
                    // { And skip any additional consecutive slashes: }
                    if ((I > Len) || (PathCharIsSlash(Filename[I]) == false)) break;
                }
            }
            else
                I = I + 1;//PathCharLength(Filename, I);
        }
        Result = I - 1;
        return Result;
    }

    // { \ }
    // { Note: Test this before 'x:' since '\:stream' means access stream 'stream'
    //   on the root directory of the current drive, not access drive '\:' }
    if ((Len >= 1) && PathCharIsSlash(Filename[0]))
    {
        if (IncludeSignificantSlash)
            Result = 1;
        else
            Result = 0;
        return Result;
    }

    //  { x: }
    if (Len > 0)
    {
        I = 1;//PathCharLength(Filename, 0);
        if ((I < Len) && (Filename[I] == L':'))
        {
            if (IncludeSignificantSlash && (I < Len) && PathCharIsSlash(Filename[I + 1]))
            {
                Result = I + 2;
            }
            else
            {
                Result = I + 1;
            }
            return Result;
        }
    }

    Result = 0;

    return Result;
}


unsigned int Path::PathPathPartLength(const wstring Filename, const bool IncludeSlashesAfterPath)
{
    //{ Returns length of the path portion of Filename, or 0 if there is no path
    //  portion.
    //  Note these differences from Delphi's ExtractFilePath function:
    //  - The result will never be less than what PathDrivePartLength returns.
    //    If you pass a UNC root path, e.g. '\\server\share', it will return the
    //    length of the entire string, NOT the length of '\\server\'.
    //  - If you pass in a filename with a reference to an NTFS alternate data
    //    stream, e.g. 'abc:def', it will return the length of the entire string,
    //    NOT the length of 'abc:'. }

    unsigned int LastCharToKeep, I;

    unsigned int Result = PathDrivePartLengthEx(Filename, true);
    LastCharToKeep = Result;
    size_t Len = Filename.length();
    I = Result;
    while (I < Len)
    {
        if (PathCharIsSlash(Filename[I]) == true)
        {
            if (IncludeSlashesAfterPath == true)
            {
                Result = I;
            }
            else
            {
                Result = LastCharToKeep;
            }
            I++;
        }
        else
        {
            I = I + 1;//PathCharLength(Filename, I);
            LastCharToKeep = I - 1;
        }
    }

    return Result;
}

wstring Path::PathExtractPath(const wstring Filename)
{
    //{ Returns the path portion of Filename (e.g. 'c:\dir\'). If Filename contains
    //  no drive part or slash, the result will be an empty string.
    //  This function is essentially the opposite of PathExtractName. }
    unsigned int I;

    I = PathPathPartLength(Filename, true);
    return wstring(Filename.c_str(), I);
}


/*
bool Path::PathCharIsTrailByte(const wstring S, const unsigned int Index)
//{ Returns False if S[Index] is a single byte character or a lead byte.
//  Returns True otherwise (i.e. it must be a trail byte). }
{
  int I;

  I = 0;
  while (I <= Index)
  {
    if (I == Index)
    {
      return false;
    }
    I = I + 1;//PathCharLength(S, I);
  }
  return true;
}
*/

unsigned int Path::PathDrivePartLength(const wstring Filename)
{
    return PathDrivePartLengthEx(Filename, false);
}

wstring Path::PathExtractDrive(const wstring Filename)
//{ Returns the drive portion of Filename (either 'x:' or '\\server\share'),
//  or an empty string if there is no drive portion. }
{
    unsigned int L;
    wstring Result;

    L = PathDrivePartLength(Filename);
    if (L == 0)
        Result = L"";
    else
        Result = Filename.substr(0, L);

    return Result;
}


wstring Path::PathChangeExt(const wstring Filename, const wstring Extension)
//{ Takes Filename, removes any existing extension, then adds the extension
//  specified by Extension and returns the resulting string. }
{
    unsigned int I;
    wstring Result;

    I = PathExtensionPos(Filename);

    if (I == 0)
        Result = Filename + Extension;
    else
        Result = Filename.substr(0, I) + Extension;

    return Result;
}

unsigned int Path::PathExtensionPos(const wstring Filename)
//{ Returns index of the last '.' character in the filename portion of Filename,
//  or 0 if there is no '.' in the filename portion.
//  Note: Filename is assumed to NOT include an NTFS alternate data stream name
//  (i.e. 'filename:stream'). }
{
    unsigned int Result = 0;
    size_t Len = Filename.length();
    UINT I = PathPathPartLength(Filename, true) + 1;
    while (I < Len)
    {
        if (Filename[I] == '.')
        {
            Result = I;
            I++;
        }
        else
        {
            I = I + 1;//PathCharLength(Filename, I);
        }
    }

    return Result;
}


wstring Path::PathExtractExt(const wstring& s)
{
    //https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s14.html
    size_t i = s.rfind('.', s.length());
    if (i != string::npos) {
        return(s.substr(i + 1, s.length() - i));
    }

    return L"";
}

bool Path::isRoot(const std::wstring& fileName)
{
    return ::PathIsRoot(fileName.c_str());
}

bool Path::isOnSystemDrive(const std::wstring& fileName)
{
    if (!fileName.empty()) {
        wchar_t systemDir[MAX_PATH];
        UINT result = ::GetSystemDirectoryW(systemDir, sizeof(systemDir) / sizeof(systemDir[0]));

        if (result > 0 && result < MAX_PATH) {
            return ::PathIsSameRootW(systemDir, fileName.c_str());;
        }
    }

    return false;
}
