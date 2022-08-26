#include "uninstall.h"
#include <VersionHelpers.h>
#include <setupapi.h>
#include "../utils/authhelper.h"

#pragma comment(lib, "Setupapi.lib")

using namespace std;

int const Uninstaller::WM_KillFirstPhase = WM_USER + 333;
DWORD Uninstaller::UninstallExitCode;

wstring Uninstaller::UninstExeFile;
bool  Uninstaller::isSilent_ = false;

std::list<wstring> Uninstaller::DirsNotRemoved;

P_SHCreateItemFromParsingName Uninstaller::SHCreateItemFromParsingNameFunc;


RemoveDirectory1 Uninstaller::remove_directory;
Redirection Uninstaller::redirection;

Path Uninstaller::path;
Services Uninstaller::services;

Uninstaller::Uninstaller()
{
}


Uninstaller::~Uninstaller()
{
}


void Uninstaller::setUninstExeFile(const wstring &exe_file, bool bFirstPhase)
{
	UninstExeFile = exe_file;
	UninstallExitCode = 1;
}

void Uninstaller::setSilent(bool isSilent)
{
	isSilent_ = isSilent;
}


bool Uninstaller::ProcessMsgs()
{
 tagMSG Msg;

 bool Result = false;

 while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
 {
   if (Msg.message == WM_QUIT)
   {
    Result = true;
    break;
   }

   TranslateMessage(&Msg);
   DispatchMessage(&Msg);
 }

 return Result;
}

void Uninstaller::ProcessDirsNotRemoved()
{

}


bool Uninstaller::DeleteDirProc(const bool DisableFsRedir, const wstring DirName)
{
 return remove_directory.DeleteDir(DisableFsRedir, DirName, &DirsNotRemoved/*PDeleteDirData(Param)^.DirsNotRemoved*/, nullptr);
}


bool Uninstaller::DeleteFileProc(const bool DisableFsRedir, const wstring FileName)
{
 bool Result;

 Log::instance().out(L"Deleting file: " + FileName);
 Result = redirection.DeleteFileRedir(DisableFsRedir, FileName);
 if (Result==false)
 {
  //char buffer[100];
  //sprintf (buffer, "Failed to delete the file; it may be in use %lu",GetLastError());

  //Log::instance().trace(string(buffer)+"\n");
 }
 return Result;
}


void Uninstaller::DelayDeleteFile(const bool DisableFsRedir, const wstring Filename,
  const unsigned int MaxTries, const unsigned int FirstRetryDelayMS, const unsigned int SubsequentRetryDelayMS)
//{ Attempts to delete Filename up to MaxTries times, retrying if the file is
//  in use. It sleeps FirstRetryDelayMS msec after the first try, and
//  SubsequentRetryDelayMS msec after subsequent tries. }
{
  for (unsigned int I = 0; I < MaxTries; I++)
  {
    if (I == 1)
    {
     Sleep(FirstRetryDelayMS);
    }
    else
    {
     if (I > 1)
     {
      Sleep(SubsequentRetryDelayMS);
     }

     DWORD error;
     error = GetLastError();
     if (redirection.DeleteFileRedir(DisableFsRedir, Filename) ||
         (error == ERROR_FILE_NOT_FOUND) ||
         (error == ERROR_PATH_NOT_FOUND))
     {
      break;
     }
    }

  }
}

void Uninstaller::DeleteUninstallDataFiles()
{
 //HWND ProcessWnd;
 //DWORD ProcessID;
 //HANDLE Process;


 Log::instance().out("Deleting Uninstall data files.");

 // { Truncate the .dat file to zero bytes just before relinquishing exclusive
//    access to it }
//  try
 //   UninstLogFile.Seek(0);
 //   UninstLogFile.Truncate;
 // except
 //   { ignore any exceptions, just in case }
 // end;
 // FreeAndNil(UninstLogFile);

//  { Delete the .dat and .msg files }
//  DeleteFile(UninstDataFile.c_str());
 // DeleteFile(UninstMsgFile);

//  { Tell the first phase to terminate, then delete its .exe }
  /*if (FirstPhaseWnd != nullptr)
  {
  //  if (InitialProcessWnd !=0)
  //  {
//      { If the first phase respawned, wait on the initial process }
  //    ProcessWnd = InitialProcessWnd;
  //  }
   // else
      {
      ProcessWnd = FirstPhaseWnd;
      }
    ProcessID = 0;
    if (GetWindowThreadProcessId(ProcessWnd, &ProcessID) != 0)
    {
      Process = OpenProcess(SYNCHRONIZE, false, ProcessID);
    }
    else
    {
      Process = nullptr;  //{ shouldn't get here }
    }
    SendNotifyMessage(FirstPhaseWnd, WM_KillFirstPhase, 0, 0);
    if (Process != nullptr)
    {
      WaitForSingleObject(Process, INFINITE);
      CloseHandle(Process);
    }
//    { Sleep for a bit to allow pre-Windows 2000 Add/Remove Programs to finish
//      bringing itself to the foreground before we take it back below. Also
//      helps the DelayDeleteFile call succeed on the first try. }
   // if not Debugging then
   //   Sleep(500);
  }*/
  UninstallExitCode = 0;
  DelayDeleteFile(false, UninstExeFile, 13, 50, 250);
 // if Debugging then
 //   DebugNotifyUninstExe('');
 // { Pre-Windows 2000 Add/Remove Programs will try to bring itself to the
 //   foreground after the first phase terminates. Take it back. }
//  Application.BringToFront;
}

typedef void (*P_DeleteUninstallDataFilesProc)();

bool Uninstaller::PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc,const wstring &path_for_installation)
{
    Log::instance().out(L"Starting the uninstallation process.");

    CoInitialize(nullptr);
    SHCreateItemFromParsingNameFunc = reinterpret_cast<P_SHCreateItemFromParsingName>(GetProcAddress(GetModuleHandle(L"shell32.dll"), "SHCreateItemFromParsingName"));

    TRegView RegView;

    RegView = rvDefault; // install_mode.GetInstallDefaultRegView();

    HKEY RootKey = HKEY_LOCAL_MACHINE;

    wstring str = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ApplicationInfo::instance().getId() + L"_is1";

    LSTATUS ret = Registry::RegDeleteKeyIncludingSubkeys1(RegView, RootKey, str.c_str());

    if ((ret == ERROR_SUCCESS) || (ret == ERROR_FILE_NOT_FOUND))
    {

    };



    remove_directory.DelTree(false, path_for_installation, true, true, true, false, nullptr, nullptr, nullptr);

    //C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Windscribe\Windscribe.lnk
    /*
    std::ifstream  stream(UninstDataFile.c_str(), std::ifstream::binary);

    if(stream)
      {
       wchar_t str_buf[1000];
       memset(str_buf,0,sizeof(str_buf));

       stream.seekg (0, stream.end);
       long long size = stream.tellg();

       stream.seekg (0, stream.beg);

       unsigned short len;

       while(stream.tellg()<size)
        {
         stream.read(reinterpret_cast<char*>(&len),sizeof(unsigned short));

         if(len>=1000)
          {
           Log::instance().trace(L"Reading of the list of files. len >= 1000.");
           break;
          }


         stream.read(reinterpret_cast<char*>(&str_buf),len*2);

         //file_list.push_back(str_buf);
         str_buf[len]=0;

         if(path.PathExtractExt(str_buf).empty()==false)
          {
           FileDelete(str_buf,false,true,false);
          }
         else
          {
           remove_directory.DelTree(false,str_buf,true,false,false,false,DeleteDirProc, DeleteFileProc, nullptr);
          }
         uninstall_progress->setProgress(str_buf);
        }

       stream.close();
      }
   */


    Log::instance().out(L"Deleting shortcuts.");

    PathsToFolders paths_to_folders;
    wstring  common_desktop = paths_to_folders.GetShellFolder(true, sfDesktop, false);


    wstring group = paths_to_folders.GetShellFolder(true, sfPrograms, false);

    redirection.DeleteFileRedir(false, common_desktop + L"\\" + ApplicationInfo::instance().getName() + L".lnk");
    remove_directory.DelTree(false, group + L"\\" + ApplicationInfo::instance().getName(), true, true, true, false, nullptr, nullptr, nullptr);


    if (DeleteUninstallDataFilesProc != nullptr)
    {
        DeleteUninstallDataFilesProc();
        //{ Now that uninstall data is deleted, try removing the directories it
        //was in that couldn't be deleted before. }
        ProcessDirsNotRemoved();
    }

    CoUninitialize();

    Log::instance().out(L"Uninstallation process succeeded.");

    return true;
}


void Uninstaller::RunSecondPhase()
{
	Log::instance().out(L"Original Uninstall EXE: " + UninstExeFile);

	if (!InitializeUninstall())
	{
		Log::instance().out(L"return after InitializeUninstall()");
		PostQuitMessage(0);
		return;
	}

	unsigned long iResultCode;

	if (services.serviceExists(L"WindscribeService"))
	{
		services.simpleStopService(L"WindscribeService", true, true);
		services.simpleDeleteService(L"WindscribeService");
	}

	wstring path_for_installation;

	path_for_installation = path.PathExtractDir(UninstExeFile);

	Log::instance().out(L"Running uninstall second phase with path: %s", path_for_installation.c_str());

	AuthHelper::removeRegEntriesForAuthHelper(path_for_installation);

	Process process;

	if (!isSilent_)
	{
		Log::instance().out(L"turn off firewall: %s", path_for_installation.c_str());
		// turn off firewall
		process.InstExec(path_for_installation + L"\\WindscribeService.exe", L"-firewall_off", path_for_installation, ewWaitUntilTerminated, SW_HIDE, iResultCode);
	}

	Log::instance().out(L"kill openvpn process");
	// kill openvpn processes
	process.InstExec(L"taskkill", L"/f /im windscribeopenvpn.exe", L"", ewWaitUntilTerminated, SW_HIDE, iResultCode);

	Log::instance().out(L"uninstall tap adapter");
	// uninstall tap-adapter
	process.InstExec(path_for_installation+L"\\tap\\tapinstall.exe", L"remove tapwindscribe0901", path_for_installation+L"\\tap", ewWaitUntilTerminated, SW_HIDE, iResultCode);

	Log::instance().out(L"uninstall wintun adapter");
	// uninstall wintun-adapter
	process.InstExec(path_for_installation + L"\\wintun\\tapinstall.exe", L"remove windtun420", path_for_installation + L"\\wintun", ewWaitUntilTerminated, SW_HIDE, iResultCode);

	Log::instance().out(L"uninstall split tunnel driver");
	// uninstall split tunnel driver

	wstring installCmd = L"DefaultUninstall 132 ";
	installCmd += path_for_installation + L"\\splittunnel\\windscribesplittunnel.inf";
	InstallHinfSection(nullptr, NULL, installCmd.c_str(), NULL);

	Log::instance().out(L"perform uninstall");
	PerformUninstall(&DeleteUninstallDataFiles, path_for_installation);

	DeleteFile(wstring(path_for_installation+L"\\log\\7zr.log").c_str());
	DeleteFile(wstring(path_for_installation+L"\\log\\windscribe_installer.log").c_str());

	//remove windscribeservice logs
	DeleteFile(wstring(path_for_installation+L"\\windscribeservice.log").c_str());
	DeleteFile(wstring(path_for_installation+L"\\windscribeservice_prev.log").c_str());

	Log::instance().out(L"remove directory");
	remove_directory.DelTree(false,path_for_installation+L"\\log", true, true, true,false, nullptr, nullptr, nullptr);

	// open uninstall screen in browser
	wstring userId;
	wstring urlStr;
	if (Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2\\", L"userId", userId)==false) userId = L"";
	if (userId != L"")
	{
		urlStr = L"https://windscribe.com/uninstall/desktop?user=" + userId;
		ShellExecute(nullptr, L"open", urlStr.c_str(), L"", L"", SW_SHOW);
	}

	// remove any existing MAC spoofs from registry
	std::wstring subkeyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
	std::vector<std::wstring> networkInterfaceNames = Registry::regGetSubkeyChildNames(HKEY_LOCAL_MACHINE, subkeyPath.c_str());

	for (std::wstring networkInterfaceName : networkInterfaceNames)
	{
		std::wstring interfacePath = subkeyPath + networkInterfaceName;

		if (Registry::regHasSubkeyProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed"))
		{
			Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"NetworkAddress");
			Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed");
		}
	}

	Log::instance().out(L"remove_directory.RemoveDir %s", path_for_installation.c_str());
	remove_directory.RemoveDir(path_for_installation.c_str());

	Log::instance().out(L"remove_directory.RemoveDir finished");

	if (!isSilent_)
	{
		MessageBox(nullptr,
			reinterpret_cast<LPCWSTR>(L"Windscribe was successfully removed from your computer."),
			reinterpret_cast<LPCWSTR>(L"Windscribe Uninstall"),
			MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
	}
	PostQuitMessage(0);
	Log::instance().out(L"End");
}


bool Uninstaller::InitializeUninstall()
{
	if (isSilent_) {
		return true;
	}

    HWND hwnd = ApplicationInfo::getAppMainWindowHandle();

	while (hwnd)
	{
		if (MessageBox(nullptr,
			reinterpret_cast<LPCWSTR>(L"Close Windscribe to continue. Please note, your connection will not be protected while the application is off."),
			reinterpret_cast<LPCWSTR>(L"Windscribe Uninstall"),
			MB_ICONINFORMATION | MB_RETRYCANCEL | MB_SETFOREGROUND) == IDCANCEL)
		{
			return false;
		}

        hwnd = ApplicationInfo::getAppMainWindowHandle();
	}

	return true;
}


//{ Attempt to unpin a shortcut. Returns True if the shortcut was successfully
//  removed from the list of pinned items and/or the taskbar, or if the shortcut
//  was not pinned at all. http://msdn.microsoft.com/en-us/library/bb774817.aspx }
bool Uninstaller::UnpinShellLink(const wstring Filename)
{
  IShellItem *ShellItem = nullptr;
  IStartMenuPinnedList *StartMenuPinnedList = nullptr;

  bool Result = false;
  ShellItem = nullptr;
  StartMenuPinnedList = nullptr;

  wstring Filename1 = path.PathExpand(Filename);


  if (IsWindowsVistaOrGreater() && //only attempt on Windows Vista and newer just to be sure
     (SHCreateItemFromParsingNameFunc!=nullptr) &&
     SUCCEEDED(SHCreateItemFromParsingNameFunc(Filename1.c_str(), nullptr, IID_IShellItem, reinterpret_cast<void**>(&ShellItem))) &&
     SUCCEEDED(CoCreateInstance(CLSID_StartMenuPin, nullptr, CLSCTX_INPROC_SERVER, IID_IStartMenuPinnedList, reinterpret_cast<void**>(&StartMenuPinnedList))))
  {
    Result = (StartMenuPinnedList->RemoveFromList(ShellItem) == S_OK);
  }
  else
  {
   Result = true;
  }


 if (StartMenuPinnedList != nullptr)
    {
     StartMenuPinnedList->Release();
    }

 if (ShellItem != nullptr)
    {
     ShellItem->Release();
    }

 return Result;
}
