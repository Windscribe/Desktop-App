#include "process1.h"

using namespace std;

Directory Process::dir;
Redirection Process::redirection;

Process::Process()
{

}

//D:\DISTR\issrc\Projects\ISPP\InstFunc.pas
void Process::HandleProcessWait(HANDLE ProcessHandle, const TExecWait Wait, unsigned long &ResultCode)
{
 /*if(Wait == ewWaitUntilIdle)
    {
      while(1)
      {
       if(WaitForInputIdle(ProcessHandle, 50) != WAIT_TIMEOUT) break;
      }
    }

 if(Wait == ewWaitUntilTerminated)
    {
     while(1)
      {
       DWORD ret;
       ret = MsgWaitForMultipleObjects(1, &ProcessHandle, false, INFINITE, QS_ALLINPUT);
      if(ret != (WAIT_OBJECT_0+1)) break;
      }
   }*/

	WaitForSingleObject(ProcessHandle, INFINITE);
   // { Get the exit code. Will be set to STILL_ACTIVE if not yet available }
 if(GetExitCodeProcess(ProcessHandle, &ResultCode)==false)
    {
      ResultCode = 0xFFFFFFFF;  //{ just in case }
    }

 CloseHandle(ProcessHandle);
}



//D:\DISTR\issrc\Projects\ISPP\InstFunc.pas
bool Process::InstExec(const wstring Filename, const wstring Params,
  wstring WorkingDir, const TExecWait Wait, const WORD ShowCmd,
  unsigned long &ResultCode)
{
  wstring CmdLine;
  STARTUPINFO StartupInfo;
  PROCESS_INFORMATION ProcessInfo;
  bool Result = true;

  if (Filename == L">")
  {
   CmdLine = Params;
  }
  else
  {
    CmdLine = L"\"" + Filename + L"\"";
    if (Params != L"")
    {
      CmdLine = CmdLine + L" " + Params;
    }

    if(WorkingDir == L"")
    {
      WorkingDir = extractFileDir(Filename);
    }
  }

  ZeroMemory(&StartupInfo, sizeof(StartupInfo));
  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = ShowCmd;
  if (WorkingDir == L"")
  {
   // Obtain location of system directory eg C:\Windows\System32
    TCHAR systemDirPath[MAX_PATH];
    GetSystemDirectory(systemDirPath, sizeof(systemDirPath)/sizeof(TCHAR));
    WorkingDir = systemDirPath;
  }


  Result = CreateProcess(nullptr, const_cast<wchar_t*>(CmdLine.c_str()),
      nullptr, nullptr,
      false, CREATE_DEFAULT_ERROR_MODE, nullptr,
      WorkingDir.c_str(), &StartupInfo, &ProcessInfo);



  if(Result==false)
  {
   ResultCode = 0;
   return Result;
  }

//  { Don't need the thread handle, so close it now }
  CloseHandle(ProcessInfo.hThread);
  HandleProcessWait(ProcessInfo.hProcess, Wait, ResultCode);

  return Result;
}

std::wstring Process::extractFileDir(std::wstring path)
{
	string path1 = string(path.begin(), path.end());

#ifdef _WIN32

	size_t pos = path1.find_last_of("\\/");
	string dir_str = (std::string::npos == pos)
		? ""
		: path1.substr(0, pos);

#else
	char path2[500];
	strncpy(path2, path1.c_str(), 500);

	char *dir_str = dirname(path2);

#endif // _WIN32

	wstring dir_str1 = wstring(dir_str.begin(), dir_str.end());

	return dir_str1;

}