#ifndef DIRECTORIES_OF_A_WINDOWS_H
#define DIRECTORIES_OF_A_WINDOWS_H
//#include "version_os.h"
#include "path.h"
#include <string>
#include "directory.h"
#include "../Utils/reg_view.h"

class DirectoriesOfAWindows
{
 private:
   Directory dir;
   Path path;
   //VersionOS ver;
   const std::wstring NEWREGSTR_PATH_SETUP = L"Software\\Microsoft\\Windows\\CurrentVersion";

 public:

   std::wstring ProgramFiles32Dir, ProgramFiles64Dir;
   std::wstring SystemDrive;
   std::wstring WinDir;

   void InitMainNonSHFolderConsts();

   std::wstring GetWinDir();
   std::wstring GetEnv(const std::wstring EnvVar);

   std::wstring GetPath(const TRegView RegView, const wchar_t *Name);
   bool AdjustLength(wchar_t *S, unsigned int &len, const unsigned int Res);
   std::wstring GetTempDir();

   DirectoriesOfAWindows();
   ~DirectoriesOfAWindows();
};

#endif // DIRECTORIES_OF_A_WINDOWS_H
