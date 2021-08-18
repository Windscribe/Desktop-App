#include "directories_of_a_windows.h"
#include "../Utils/version_os.h"

using namespace std;

DirectoriesOfAWindows::DirectoriesOfAWindows()
{

}

DirectoriesOfAWindows::~DirectoriesOfAWindows()
{

}
void DirectoriesOfAWindows::InitMainNonSHFolderConsts()
{
//{ Read Windows and Windows System dirs }
  WinDir = GetWinDir();

// { Get system drive }
	SystemDrive = GetEnv(L"SystemDrive"); //  {don't localize}

  if(SystemDrive == L"")
  {
   SystemDrive = path.PathExtractDrive(WinDir);

   if (SystemDrive == L"")
    {
   //   { In some rare case that PathExtractDrive failed, just default to C }
      SystemDrive = L"C:";
    }
  }

  //{ Get 32-bit Program Files and Common Files dirs }
  ProgramFiles32Dir = GetPath(rv32Bit, L"ProgramFilesDir");
  if (ProgramFiles32Dir == L"")
  {
    ProgramFiles32Dir = SystemDrive + L"\\Program Files";//  {don't localize}
  }


 // { Get 64-bit Program Files and Common Files dirs }
  if (VersionOS::isWin64() == true)
  {
    ProgramFiles64Dir = GetPath(rv64Bit, L"ProgramFilesDir");
    if (ProgramFiles64Dir ==L"")
    {
     // InternalError('Failed to get path of 64-bit Program Files directory');
    }

  }

}


wstring DirectoriesOfAWindows::GetWinDir()
//{ Returns fully qualified path of the Windows directory. Only includes a
//  trailing backslash if the Windows directory is the root directory. }
{
 wchar_t Buf[MAX_PATH];

 GetWindowsDirectory(Buf, sizeof(Buf) / sizeof(Buf[0]));
 return Buf;
}

wstring DirectoriesOfAWindows::GetEnv(const wstring EnvVar)
{
//{ Gets the value of the specified environment variable. (Just like TP's GetEnv) }

 DWORD Res;

 unsigned int len=255;
 wchar_t *Result = new TCHAR[len];

  while(1)
  {
    Res = GetEnvironmentVariable(EnvVar.c_str(), Result, len);
    if(Res==0)
    {
     Result[0] = 0;
     break;
    }

   if(AdjustLength(Result,len, Res)==true) break;
  }

  wstring str = Result;
  delete [] Result;

  return str;
}

wstring DirectoriesOfAWindows::GetPath(const TRegView RegView, const wchar_t *Name)
{
 Registry reg;

 HKEY H;
 wstring Result;

 if(reg.RegOpenKeyExView(RegView, HKEY_LOCAL_MACHINE, NEWREGSTR_PATH_SETUP.c_str(), 0,KEY_QUERY_VALUE, H) == ERROR_SUCCESS)
  {
    if(reg.RegQueryStringValue1(H, Name, Result)==false)
    {
     Result = L"";
    }
    RegCloseKey(H);
  }
 else
  {
   Result = L"";
  }

 return Result;
}


bool DirectoriesOfAWindows::AdjustLength(wchar_t *S, unsigned int &len, const unsigned int Res)
//{ Returns True if successful. Returns False if buffer wasn't large enough,
//  and called AdjustLength to resize it. }
{
  bool ret;

  ret = Res < len;

  if(ret==false)
  {
   delete [] S;

   S = new wchar_t[Res];
   len = Res;
  }

  return ret;
}

wstring DirectoriesOfAWindows::GetTempDir()
{
//{ Returns fully qualified path of the temporary directory, with trailing
//  backslash. This does not use the Win32 function GetTempPath, due to platform
//  differences. }
  wstring Result;
  Result = GetEnv(L"TMP");
  if ((Result != L"") && dir.DirExists(Result))
  {
    goto A;
  }

  Result = GetEnv(L"TEMP");
  if ((Result != L"") && dir.DirExists(Result))
  {
    goto A;
  }

  //  { Like Windows 2000's GetTempPath, return USERPROFILE when TMP and TEMP
  //    are not set }
    Result = GetEnv(L"USERPROFILE");
    if ((Result != L"") && dir.DirExists(Result))
    {
      goto A;
    }

  Result = GetWinDir();

  A: Result = path.AddBackslash(path.PathExpand(Result));

  return Result;
}
