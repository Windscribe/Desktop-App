#include "paths_to_folders.h"
#include <versionhelpers.h>

using namespace std;

PathsToFolders::PathsToFolders()
{
    for (int i=0;i<2;i++)
    {
     for (int j=0;j<11;j++)
     {
      ShellFoldersRead[i][j]=false;
     }

    }
}

wstring PathsToFolders::GetShellFolderByCSIDL(int Folder,bool const Create)
{
 // CSIDL_FLAG_CREATE = $8000;
 //SHGFP_TYPE_CURRENT = nullptr;

  wstring Result;
  HRESULT Res;
  wchar_t Buf[MAX_PATH];

  if (Create)
    Folder = Folder | CSIDL_FLAG_CREATE;

//  { Work around a nasty bug in Windows Vista (still present in SP1) and
//    Windows Server 2008: When a folder ID resolves to the root directory of a
//    drive ('X:\') and the CSIDL_FLAG_CREATE flag is passed, SHGetFolderPath
//    fails with code 0x80070005.
//    So on Vista only, first try calling the function without CSIDL_FLAG_CREATE.
//    If and only if that fails, call it again with the flag.
//    Note: The calls *must* be issued in this order; if it's called with the
//    flag first, it seems to permanently cache the failure code, causing future
//    calls that don't include the flag to fail as well. }
  if (IsWindowsVistaOrGreater() && ((Folder & CSIDL_FLAG_CREATE) != 0) )
    Res = SHGetFolderPath(nullptr, Folder & ~CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, Buf);
  else
    Res = E_FAIL;  //{ always issue the call below }

  if(Res != S_OK)
  {
   Res = SHGetFolderPath(nullptr, Folder, nullptr, SHGFP_TYPE_CURRENT, Buf);
  }

  if(Res == S_OK)
  {
   Result = path.RemoveBackslashUnlessRoot(path.PathExpand(Buf));
  }
  else
  {
   Result = L"";
    //LogFmt('Warning: SHGetFolderPath failed with code 0x%.8x on folder 0x%.4x',
    //  [Res, Folder]);
  }

 return Result;
}

wstring PathsToFolders::GetShellFolder(const bool Common, const TShellFolderID ID, bool ReadOnly)
{
  int FolderIDs[2][11] = {
   // { User }
    {CSIDL_DESKTOPDIRECTORY, CSIDL_STARTMENU, CSIDL_PROGRAMS, CSIDL_STARTUP,
     CSIDL_SENDTO, CSIDL_FONTS, CSIDL_APPDATA, CSIDL_PERSONAL,
     CSIDL_TEMPLATES, CSIDL_FAVORITES, CSIDL_LOCAL_APPDATA},
   // { Common }
      {CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_COMMON_STARTMENU, CSIDL_COMMON_PROGRAMS, CSIDL_COMMON_STARTUP,
     CSIDL_SENDTO, CSIDL_FONTS, CSIDL_COMMON_APPDATA, CSIDL_COMMON_DOCUMENTS,
     CSIDL_COMMON_TEMPLATES, CSIDL_COMMON_FAVORITES, CSIDL_LOCAL_APPDATA}};

  wstring ShellFolder;

  if (ShellFoldersRead[Common][ID]==false)
    {
//    { Note: Must pass Create=True or else SHGetFolderPath fails if the
//      specified CSIDL is valid but doesn't currently exist. }
    ShellFolder = GetShellFolderByCSIDL(FolderIDs[Common][ID], !ReadOnly);
    ShellFolders[Common][ID] = ShellFolder;
    if ((ReadOnly==false) || (ShellFolder != L""))
      ShellFoldersRead[Common][ID] = true;
  }
  return ShellFolders[Common][ID];
}
