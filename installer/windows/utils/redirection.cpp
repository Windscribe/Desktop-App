#include "redirection.h"

using namespace std;

Redirection::Redirection()
{

}
bool Redirection::RemoveDirectoryReDir(const bool DisableFsRedir, const wstring Filename)
{
 TPreviousFsRedirectionState PrevState;
 PrevState.OldValue = nullptr;
 PrevState.DidDisable = false;
 DWORD ErrorCode;
 bool Result;

 if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
    return false;
  }
 Result = RemoveDirectory(Filename.c_str());
 ErrorCode = GetLastError();

 RestoreFsRedirection(PrevState);

 SetLastError(ErrorCode);

 return Result;
}
bool Redirection::SetFileAttributesReDir(const bool DisableFsRedir, const wstring Filename, const DWORD Attrib)
{
 TPreviousFsRedirectionState PrevState;
 PrevState.OldValue = nullptr;
 PrevState.DidDisable = false;

 DWORD ErrorCode;
 bool Result;

 if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
 {
  return false;
 }

 Result = SetFileAttributes(Filename.c_str(), Attrib);
 ErrorCode = GetLastError();

 RestoreFsRedirection(PrevState);

 SetLastError(ErrorCode);

 return Result;
}



bool Redirection::IsDirectoryAndNotReparsePointRedir(const bool DisableFsRedir, const wstring Name)
{
  TPreviousFsRedirectionState PrevState;
  PrevState.OldValue = nullptr;
  PrevState.DidDisable = false;
  bool Result=true;

  if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
   return Result;
  }
 // try
 // {
   Result = IsDirectoryAndNotReparsePoint(Name);
 // }
 // __finally
 // {
   RestoreFsRedirection(PrevState);
 // }

  return Result;
}

bool Redirection::DeleteFileRedir(const bool DisableFsRedir, const wstring Filename)
{
 TPreviousFsRedirectionState PrevState;
 PrevState.OldValue = nullptr;
 PrevState.DidDisable = false;
 DWORD ErrorCode;
 bool Result;

 if(DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
   return false;
  }

// try{
    Result = DeleteFile(Filename.c_str());
    ErrorCode = GetLastError();
//    }
//  __finally
 // {
   RestoreFsRedirection(PrevState);
 // }
  SetLastError(ErrorCode);

  return Result;
}


bool Redirection::MoveFileExRedir(const bool DisableFsRedir,
  const wstring ExistingFilename, const wstring NewFilename, const DWORD Flags)
{
  const wchar_t *NewFilenameP;
  TPreviousFsRedirectionState PrevState;
  PrevState.OldValue = nullptr;
  PrevState.DidDisable = false;
  DWORD ErrorCode;
  bool Result=true;

  if ((NewFilename == L"") && ((Flags & MOVEFILE_DELAY_UNTIL_REBOOT) != 0))
  {
   NewFilenameP = nullptr;
  }
  else
  {
   NewFilenameP = NewFilename.c_str();
  }

  if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
   return false;
  }

 // try
 // {
   Result = MoveFileEx(ExistingFilename.c_str(), NewFilenameP, Flags);
   ErrorCode = GetLastError();
 // }
 // __finally
 // {
    RestoreFsRedirection(PrevState);
 // }
  SetLastError(ErrorCode);

  return Result;
}

//D:\DISTR\issrc\Projects\ISPP\RedirFunc.pas
bool Redirection::CreateProcessRedir(const wchar_t *lpApplicationName, const wchar_t *lpCommandLine,
  const LPSECURITY_ATTRIBUTES lpProcessAttributes,const LPSECURITY_ATTRIBUTES lpThreadAttributes,
  const bool bInheritHandles, const DWORD dwCreationFlags,
  const void *lpEnvironment, const wchar_t *lpCurrentDirectory,
  LPSTARTUPINFO lpStartupInfo,
  LPPROCESS_INFORMATION lpProcessInformation)
{

  DWORD ErrorCode;
  bool Result;

  //TPreviousFsRedirectionState PrevState;
 // PrevState.OldValue = nullptr;
 // PrevState.DidDisable = false;
 // if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
//  {
//    Result = false;
//    return Result;
//  }

  //try{
    Result = CreateProcess(lpApplicationName, const_cast<wchar_t*>(lpCommandLine),
      lpProcessAttributes, lpThreadAttributes,
      bInheritHandles, dwCreationFlags, const_cast<void*>(lpEnvironment),
      lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
    ErrorCode = GetLastError();
 // }
  //__finally
 // {
    //RestoreFsRedirection(PrevState);
 // }
  SetLastError(ErrorCode);

  return Result;
}


//D:\DISTR\issrc\Projects\ISPP\RedirFunc.pas
bool Redirection::DisableFsRedirectionIf(const bool Disable, TPreviousFsRedirectionState PreviousState)
{
//{ If Disable is false, the function does not change the redirection state and
//  always returns true.
//  If Disable is true, the function attempts to disable WOW64 file system
//  redirection, so that c:\windows\system32 goes to the 64-bit System directory
//  instead of the 32-bit one.
//  Returns true if successful, false if not (which normally indicates that
//  either the user is running 32-bit Windows, or a 64-bit version prior to
//  Windows Server 2003 SP1). For extended error information when false is
//  returned, call GetLastError. }

  bool Result = false;

  PreviousState.DidDisable = false;
  if (Disable==false)
  {
    Result = true;
  }
  else
  {
   if(FsRedirectionFunctionsAvailable==true)
    {
//      { Note: Disassembling Wow64DisableWow64FsRedirection and the Rtl function
//        it calls, it doesn't appear as if it can ever actually fail on 64-bit
//        Windows. But it always fails on the 32-bit version of Windows Server
//        2003 SP1 (with error code 1 - ERROR_INVALID_FUNCTION). }
      Result = Wow64DisableWow64FsRedirection(&PreviousState.OldValue);
      if (Result==true)
      {
        PreviousState.DidDisable = true;
      }
    }
    else
    {
    // { The functions do not exist prior to Windows Server 2003 SP1 }
     SetLastError(ERROR_INVALID_FUNCTION);
     Result = false;
    }
  }

 return Result;
}

void Redirection::RestoreFsRedirection(const TPreviousFsRedirectionState PreviousState)
{
//{ Restores the previous WOW64 file system redirection state after a call to
//  DisableFsRedirectionIf. There is no indication of failure (which is
//  extremely unlikely). }
 if (PreviousState.DidDisable==true)
 {
  Wow64RevertWow64FsRedirection(PreviousState.OldValue);
 }
}

DWORD Redirection::GetFileAttributesRedir(const bool DisableFsRedir, const wstring Filename)
{
 DWORD Result;
 TPreviousFsRedirectionState PrevState;
 PrevState.OldValue = nullptr;
 PrevState.DidDisable = false;
 DWORD ErrorCode;

 if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
    Result = 0xFFFFFFFF;
    return Result;
  }

 Result = GetFileAttributes(Filename.c_str());
 ErrorCode = GetLastError();

 RestoreFsRedirection(PrevState);

 SetLastError(ErrorCode);

 return Result;
}





HANDLE Redirection::FindFirstFileRedir(const bool DisableFsRedir, const wstring Filename, WIN32_FIND_DATA &FindData)
{
 TPreviousFsRedirectionState PrevState;
 PrevState.OldValue = nullptr;
 PrevState.DidDisable = false;
 DWORD ErrorCode;

 HANDLE Result;

 if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
    return  INVALID_HANDLE_VALUE;
  }
  //try{
    Result = FindFirstFile(Filename.c_str(), &FindData);
    ErrorCode = GetLastError();
 //}
  //__finally
  //{
   RestoreFsRedirection(PrevState);
  //}
  SetLastError(ErrorCode);

  return Result;
}

bool Redirection::DirExistsRedir(const bool DisableFsRedir, const wstring Filename)
{
  TPreviousFsRedirectionState PrevState;
  PrevState.OldValue = nullptr;
  PrevState.DidDisable = false;
  DWORD ErrorCode;
  bool Result;

  if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
     return false;
  }


  Result = dir.DirExists(Filename);
  ErrorCode = GetLastError();

  RestoreFsRedirection(PrevState);

  SetLastError(ErrorCode);

  return Result;
}

bool Redirection::CreateDirectoryRedir(const bool DisableFsRedir, const wstring Filename)
{
  TPreviousFsRedirectionState PrevState;
  PrevState.OldValue = nullptr;
  PrevState.DidDisable = false;
  DWORD ErrorCode;
  bool Result=true;

  if (DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
    return false;
  }

  Result = CreateDirectory(Filename.c_str(),nullptr);
  ErrorCode = GetLastError();

  RestoreFsRedirection(PrevState);

  SetLastError(ErrorCode);

  return Result;
}

bool Redirection::IsDirectoryAndNotReparsePoint(const wstring Name)
{
//{ Returns True if the specified directory exists and is NOT a reparse point. }
 DWORD Attr;

 Attr = GetFileAttributes(Name.c_str());
 return (Attr != 0xFFFFFFFF) && ((Attr & FILE_ATTRIBUTE_DIRECTORY) != 0) &&  ((Attr & FILE_ATTRIBUTE_REPARSE_POINT) == 0);
}



bool Redirection::AreFsRedirectionFunctionsAvailable()
{
  return FsRedirectionFunctionsAvailable;
}


bool Redirection::NewFileExists(const wstring Name)
//{ Returns True if the specified file exists.
//  This function is better than Delphi's FileExists function because it works
//  on files in directories that don't have "list" permission. There is, however,
//  one other difference: FileExists allows wildcards, but this function does
//  not. }
{
 DWORD Attr;

 Attr = GetFileAttributes(Name.c_str());
// DWORD error = GetLastError();
 return (Attr != 0xFFFFFFFF) && ((Attr & 0x00000010) == 0);
}

bool Redirection::NewFileExistsRedir(const bool DisableFsRedir, const wstring Filename)
{
  TPreviousFsRedirectionState PrevState;
  PrevState.OldValue = nullptr;
  PrevState.DidDisable = false;
  DWORD ErrorCode;
  bool Result;
  if(DisableFsRedirectionIf(DisableFsRedir, PrevState)==false)
  {
    return false;
   }
 // try
 // {
    Result = NewFileExists(Filename);
    ErrorCode = GetLastError();
 // }
//  __finally
//  {
    RestoreFsRedirection(PrevState);
 // }
  SetLastError(ErrorCode);
  return Result;
}
