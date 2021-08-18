#ifndef PROCESS1_H
#define PROCESS1_H

#include <Windows.h>
#include <string>
#include "redirection.h"


enum TExecWait {ewNoWait, ewWaitUntilTerminated, ewWaitUntilIdle};

class Process
{
 private:
    static Redirection redirection;
    static Directory dir;

    static void HandleProcessWait(HANDLE ProcessHandle, const TExecWait Wait,unsigned long &ResultCode);
	static std::wstring extractFileDir(std::wstring path);


 public:
    static bool InstExec(const std::wstring Filename, const std::wstring Params,
                         std::wstring WorkingDir, const TExecWait Wait, const WORD ShowCmd, unsigned long &ResultCode);
  Process();
};

#endif // PROCESS1_H
