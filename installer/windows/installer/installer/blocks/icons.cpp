#include "icons.h"

#include <versionhelpers.h>

#include "../settings.h"
#include "../../../Utils/applicationinfo.h"
#include "../../../Utils/directory.h"
#include "../../../Utils/path.h"
#include "../../../Utils/registry.h"

using namespace std;

Icons::Icons(bool isCreateShortcut, double weight) : IInstallBlock(weight, L"icons"), isCreateShortcut_(isCreateShortcut)
{
}

Icons::~Icons()
{
}

int Icons::executeStep()
{
    const wstring& installPath = Settings::instance().getPath();
    wstring uninstallExeFilename = Path::AddBackslash(installPath) + ApplicationInfo::instance().getUninstall();

    wstring common_desktop = pathsToFolders_.GetShellFolder(true, sfDesktop, false);

	if (isCreateShortcut_)
	{
		//C:\\Users\\Public\\Desktop\\Windscribe.lnk
		CreateAnIcon(common_desktop + L"\\" + ApplicationInfo::instance().getName(), L"", Path::AddBackslash(installPath) + L"WindscribeLauncher.exe", L"", installPath, L"", 0, 1, 0, true);
	}

	wstring group = pathsToFolders_.GetShellFolder(true, sfPrograms, false);

	//C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Uninstall Windscribe.lnk
	CreateAnIcon(group+L"\\"+ ApplicationInfo::instance().getName() + L"\\Uninstall Windscribe",L"", uninstallExeFilename, L"", installPath,L"",0,1,0,false);

	//C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Windscribe.lnk
	CreateAnIcon(group+L"\\"+ ApplicationInfo::instance().getName() + L"\\Windscribe",L"", Path::AddBackslash(installPath) + L"\\WindscribeLauncher.exe",L"", installPath, L"", 0, 1, 0, false);
	return 100;
}


void Icons::CreateAnIcon(wstring Name, const wstring Description, const wstring Path,const wstring Parameters,
  wstring WorkingDir, wstring IconFilename, const int IconIndex, const int ShowCmd,
  const unsigned short HotKey, bool FolderShortcut)
{
	//bool BeginsWithGroup;
	wstring LinkFilename, PifFilename, UrlFilename, DirFilename, ProbableFilename, ResultingFilename;
	TMakeDirFlags Flags = mdAlwaysUninstall;
	bool FolderShortcutCreated;



	//  { Note: PathExpand removes trailing spaces, so it can't be called on
	//    Name before the extensions are appended }
	// Name = ExpandConst(Name);
	LinkFilename = Path::PathExpand(Name + L".lnk");
	DirFilename = Path::PathExpand(Name);


	// { On Windows 7, folder shortcuts don't expand properly on the Start Menu
	//  (they just show "target"), so ignore the foldershortcut flag.
	//  (Windows Vista works fine.) }
	if (FolderShortcut && IsWindows7OrGreater())
	{
		FolderShortcut = false;
	}

	ProbableFilename = LinkFilename;

	//Log::instance().trace(L"(" + name + L")" + L"Dest filename: " + ProbableFilename);

	// SetFilenameLabelText(ProbableFilename, True);
	MakeDir(false, Path::PathExtractDir(ProbableFilename).c_str(), Flags);

	// { Delete any old files first }
	DeleteFile(LinkFilename.c_str());
	DeleteFile(PifFilename.c_str());
	DeleteFile(UrlFilename.c_str());
	DeleteFolderShortcut(DirFilename.c_str());

	//Log::instance().trace(L"(" + name + L")" + L"Creating the icon.");


	// { Create the shortcut.
	//   Note: Don't call path.PathExpand on any of the paths since they may contain
	//   environment-variable strings (e.g. %SystemRoot%\...) }
	ResultingFilename = CreateShellLink(LinkFilename, Description, Path,
      Parameters, WorkingDir, IconFilename, IconIndex, ShowCmd, HotKey,
      FolderShortcut);
    FolderShortcutCreated = FolderShortcut && Directory::DirExists(ResultingFilename);


	//Log::instance().trace(L"(" + name + L")" + L"Successfully created the icon.");

	//  { Set the global flag that is checked by the Finished wizard page }
	//bool CreatedIcon = True;

	// { Notify shell of the change }
	if(FolderShortcutCreated)
		SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, ResultingFilename.c_str(), nullptr);
	else
		SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, ResultingFilename.c_str(), nullptr);

	SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, Path::PathExtractDir(ResultingFilename).c_str(), nullptr);
}


bool Icons::MakeDir(const bool DisableFsRedir,wstring Dir, const TMakeDirFlags Flags)
{
//{ Returns True if a new directory was created.
//  Note: If DisableFsRedir is True, the mdNotifyChange flag should not be
//  specified; it won't work properly. }
  DWORD ErrorCode;
  //int UninstFlags;

  bool Result = false;
  Dir = Path::RemoveBackslashUnlessRoot(Path::PathExpand(Dir));
  if (Path::PathExtractName(Dir) == L"")   //{ reached root? }
  {
    return false;
  }

  if (redirection.DirExistsRedir(DisableFsRedir, Dir))
  {
    if (Flags!=mdAlwaysUninstall)
      return false;
  }
  else
  {
    TMakeDirFlags Flags1 = mdNoUninstall;
    MakeDir(DisableFsRedir,Path::PathExtractDir(Dir), Flags1);
    //Log::instance().trace(L"(" + name + L")" + L"Creating directory: " + Dir);
    if(redirection.CreateDirectoryRedir(DisableFsRedir, Dir)==false)
    {
      ErrorCode = GetLastError();
    //  raise Exception.Create(FmtSetupMessage(msgLastErrorMessage,
    //    [FmtSetupMessage1(msgErrorCreatingDir, Dir), IntToStr(ErrorCode),
   //      Win32ErrorString(ErrorCode)]));
    }
    Result = true;
    if (Flags==mdNotifyChange)
    {
      SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, Dir.c_str(), nullptr);
      SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, Path::PathExtractDir(Dir).c_str(), nullptr);
    }
  }
  if (Flags==mdDeleteAfterInstall)
  {
  //  DeleteDirsAfterInstallList.AddObject(Dir, Pointer(Ord(DisableFsRedir)))
  }
  else
    {
     if (Flags!=mdNoUninstall)
     {
//      UninstFlags = utDeleteDirOrFiles_IsDir;

//      if (DisableFsRedir)
//        UninstFlags = UninstFlags | utDeleteDirOrFiles_DisableFsRedir;

//      if (Flags== mdNotifyChange)
//        UninstFlags = UninstFlags | utDeleteDirOrFiles_CallChangeNotify;

//      UninstLog::instance().Add(utDeleteDirOrFiles, [Dir], UninstFlags);
     }
  }


  return Result;
}

void Icons::DeleteFolderShortcut(const wstring Dir)
{
  DWORD Attr;
  wstring  DesktopIniFilename, S;

  Attr = GetFileAttributes(Dir.c_str());
   if ((Attr != 0xFFFFFFFF) && (Attr & FILE_ATTRIBUTE_DIRECTORY))
   {
  //   { To be sure this is really a folder shortcut and not a regular folder,
  //     look for a desktop.ini file specifying CLSID_FolderShortcut }
//     DesktopIniFilename = PathCombine(Dir, L"desktop.ini");
//     S = GetIniString(".ShellClassInfo", "CLSID2", L"", DesktopIniFilename);
//     if (CompareText(S, '{0AFACED1-E828-11D1-9187-B532F1E9575D}') == nullptr)
//       {
//        DeleteFile(DesktopIniFilename);
//       DeleteFile(PathCombine(Dir, 'target.lnk'));
//       SetFileAttributes(PChar(Dir), Attr and not FILE_ATTRIBUTE_READONLY);
//       RemoveDirectory(PChar(Dir));
//     }
   }
}


//D:\DISTR\issrc\Projects\InstFnc2.pas
wstring Icons::CreateShellLink(const wstring Filename, const wstring Description, const wstring ShortcutTo, const wstring Parameters,
  const wstring WorkingDir, const wstring IconFileName, const int IconIndex, const int ShowCmd,
  const WORD HotKey, bool FolderShortcut)
  {
//{ Creates a lnk file named Filename, with a description of Description, with a
//  HotKey hotkey, which points to ShortcutTo. Filename should be a full path.
//  NOTE! If you want to copy this procedure for use in your own application
//  be sure to call CoInitialize at application startup and CoUninitialize at
//  application shutdown. See the bottom of this unit for an example. But this
//  is not necessary if you are using Delphi 3 and your project already 'uses'
//  the ComObj RTL unit. }

  wstring  Result;

  CoInitialize(nullptr);

  HRESULT OleResult;
  IShellLink *pSL = nullptr;
  IPropertyStore *pPS;
  //TPropVariant *PV;
  IPersistFile *pPF;
  //LPCOLESTR WideFilename;
  wchar_t *WideFilename;
  if (FolderShortcut==true)
  {
    OleResult = CoCreateInstance(CLSID_FolderShortcut, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&pSL));
  }
  else
  {
   OleResult = E_FAIL;
  }
//http://www.cplusplus.com/forum/windows/28812/
//  { If a folder shortcut wasn't requested, or if CoCreateInstance failed
//    because the user isn't running Windows 2000/Me or later, create a normal
//    shell link instead }
  if (OleResult != S_OK)
  {
    FolderShortcut = false;
    OleResult = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,IID_IShellLink, reinterpret_cast<LPVOID*>(&pSL));
    if (OleResult != S_OK)
    {
     // RaiseOleError('CoCreateInstance', OleResult);
    }


    /*
   if (OleResult == S_OK) cout << "OK" << endl;
    if (OleResult == REGDB_E_CLASSNOTREG) cout << "Class not registered" << endl;
    if (OleResult == CLASS_E_NOAGGREGATION) cout << "No Aggregation" << endl;
    if (OleResult == E_NOINTERFACE) cout << "No interface" << endl;
    if (OleResult == E_POINTER) cout << "Pointer" << endl;
*/

  }
  pPF = nullptr;
  pPS = nullptr;
  WideFilename = nullptr;
 // try
 // {
    pSL->SetPath(ShortcutTo.c_str());

    pSL->SetArguments(Parameters.c_str());

    if (!FolderShortcut)
    {
      AssignWorkingDir(pSL, WorkingDir);
    }

    if (IconFileName != L"")
    {
//      { Work around a 64-bit Windows bug. It replaces pf32 with %ProgramFiles%
//      which is wrong. This causes an error when the user tries to change the
//      icon of the installed shortcut. Note that the icon does actually display
//      fine because it *also* stores the original 'non replaced' path in the
//      shortcut. }
     // if (IsWin64()==true)
      //  StringChangeEx(IconFileName, ExpandConst('{pf32}\'), "%ProgramFiles(x86)%\", True);

      pSL->SetIconLocation(IconFileName.c_str(), IconIndex);
    }

    pSL->SetShowCmd(ShowCmd);

    if (Description != L"")
    {
      pSL->SetDescription(Description.c_str());
    }

    if (HotKey != 0)
    {
      pSL->SetHotkey(HotKey);
    }
    /*
//    { Note: Vista and newer support IPropertyStore but Vista errors if you try to
//      commit a PKEY_AppUserModel_ID, so avoid setting the property on Vista. }
    if ((IsWindows7()==true) && ((AppUserModelID != L"") || ExcludeFromShowInNewInstall || PreventPinning))
    {
      OleResult = pSL->QueryInterface(IID_IPropertyStore, PS);
      if (OleResult != S_OK)
      {
       // RaiseOleError('IShellLink::QueryInterface(IID_IPropertyStore)', OleResult);
      }
//      { According to MSDN the PreventPinning property should be set before the ID property. In practice
//        this doesn't seem to matter - at least not for shortcuts - but do it first anyway. }
       if (PreventPinning)
       {
        pPV.vt = VT_BOOL;
        Smallint(pPV.vbool) = -1;
        OleResult = ppPS->SetValue(PKEY_AppUserModel_PreventPinning, PV);
        if (OleResult != S_OK)
        {
         // RaiseOleError('IPropertyStore::SetValue(PKEY_AppUserModel_PreventPinning)', OleResult);
        }
      }
      if (AppUserModelID != '') {
        PV.vt = VT_BSTR;
        PV.bstrVal = StringToOleStr(AppUserModelID);
        if (PV.bstrVal == nullptr)
          //OutOfMemoryError;
        try{
          OleResult = ppPS->SetValue(PKEY_AppUserModel_ID, PV);
          if (OleResult != S_OK)
            RaiseOleError('IPropertyStore::SetValue(PKEY_AppUserModel_ID)', OleResult);
        }
        __finally{
          SysFreeString(PV.bstrVal);
      }
      }
      if (ExcludeFromShowInNewInstall)
      {
        PV.vt = VT_BOOL;
        Smallint(PV.vbool) = -1;
        OleResult = pPS->SetValue(PKEY_AppUserModel_ExcludeFromShowInNewInstall, PV);
        if (OleResult != S_OK)
        {
          RaiseOleError('IPropertyStore::SetValue(PKEY_AppUserModel_ExcludeFromShowInNewInstall)', OleResult);
        }
        if (IsWindows8()) {
          PV.vt = VT_UI4;
          PV.lVal = APPUSERMODEL_STARTPINOPTION_NOPINONINSTALL;
          OleResult = pPS->SetValue(PKEY_AppUserModel_StartPinOption, PV);
          if (OleResult != S_OK)
          {
           // RaiseOleError('IPropertyStore::SetValue(PKEY_AppUserModel_StartPinOption)', OleResult);
          }
        }
      }
      OleResult = pPS->Commit;
      if (OleResult != S_OK )
      {
        RaiseOleError('IPropertyStore::Commit', OleResult);
      }
    }
*/
    OleResult = pSL->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&pPF));
    if (OleResult != S_OK)
    {
     // RaiseOleError('IShellLink::QueryInterface(IID_IPersistFile)', OleResult);
    }
//    { When creating a folder shortcut on 2000/Me, IPersistFile::Save will strip
//      off everything past the last '.' in the filename, so we keep the .lnk
//      extension on to give it something harmless to strip off. XP doesn't do
//      that, so we must remove the .lnk extension ourself. }


    /*if ((FolderShortcut==true) && (IsWindowsXP()==true))
    {
     wstring str = path.PathChangeExt(Filename, L"");
     WideFilename = reinterpret_cast<wchar_t*>(CoTaskMemAlloc(str.length()*2+2));
     wcscpy_s(WideFilename,str.length()+1,str.c_str());
    }
    else*/
    {
     WideFilename = reinterpret_cast<wchar_t*>(CoTaskMemAlloc(Filename.length()*2+2));
     wcscpy_s(WideFilename,Filename.length()+1,Filename.c_str());
    }
//reinterpret_cast<LPCOLESTR>

    if (WideFilename == nullptr)
    {
      //OutOfMemoryError;
    }

    OleResult = pPF->Save(WideFilename, true);
    if (OleResult != S_OK)
    {
     // RaiseOleError('IPersistFile::Save', OleResult);
    }

    //Result = GetResultingFilename(pPF, Filename);
    LPOLESTR CurFilename;
    IMalloc* pMalloc;
    OleResult = pPF->GetCurFile(&CurFilename);


   // { Note: Prior to Windows 2000/Me, GetCurFile succeeds but returns a nullptr
   //   pointer }
    if (SUCCEEDED(OleResult) && (CurFilename!=nullptr))
    {
      if (OleResult == S_OK)
      {
       Result = reinterpret_cast<wchar_t*>(CurFilename);
      }

      //we receive the pointer on IMalloc
      ::SHGetMalloc(&pMalloc);
      //we release memory
      pMalloc->Free(CurFilename);
      //IMalloc is not necessary any more
      pMalloc->Release();
    }

 // }
 // __finally
 // {
    if (WideFilename!=nullptr)
      CoTaskMemFree(reinterpret_cast<PVOID>(WideFilename));
    if (pPS!=nullptr)
      pPS->Release();
    if(pPF!=nullptr)
      pPF->Release();

    pSL->Release();
 //}

  CoUninitialize();
  return Result;
}


void Icons::AssignWorkingDir(IShellLink *pSL, const wstring WorkingDir)
{
//{ Assigns the specified working directory to pSL-> If WorkingDir is empty then
//  we select one ourself as best we can. (Leaving the working directory field
//  empty is a security risk.) Note: pSL->SetPath must be called first. }

 wstring Dir;
 //char Buf[1024];

 // { Try any caller-supplied WorkingDir first }
  if (WorkingDir != L"")
  {
  //  { SetWorkingDirectory *shouldn't* fail, but we might as well check }
    if (pSL->SetWorkingDirectory(WorkingDir.c_str()) == S_OK)
      return;
  }
//  { Otherwise, try to extract a directory name from the shortcut's target
//    filename. We use GetPath to retrieve the filename as it will expand any
//    environment strings. }
 /*
  if (pSL->GetPath(Buf, sizeof(Buf) / sizeof(Buf[nullptr]), TWin32FindData(nullptr^), nullptr) == S_OK)
  {
    Dir = PathExtractDir(path.PathExpand(Buf));
    if (ppSL->SetWorkingDirectory(PChar(Dir)) == S_OK)
      return;
  }

 // { As a last resort, use the system directory }
  Dir = GetSystemDir;
  */
  pSL->SetWorkingDirectory(Dir.c_str());
}