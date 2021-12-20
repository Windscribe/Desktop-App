#ifndef PATHS_TO_FOLDERS_H
#define PATHS_TO_FOLDERS_H
#include <string>
#include <ShlObj.h>
#include "path.h"
//#include "version_os.h"

enum TShellFolderID {sfDesktop, sfStartMenu, sfPrograms, sfStartup, sfSendTo,
   sfFonts, sfAppData, sfDocs, sfTemplates, sfFavorites, sfLocalAppData};

class PathsToFolders
{
 private:
    //VersionOS ver;
    Path path;
    bool ShellFoldersRead[2][11];
    std::wstring ShellFolders[2][11];

    std::wstring GetShellFolderByCSIDL(int Folder,bool const Create);

 public:


    std::wstring GetShellFolder(const bool Common, const TShellFolderID ID, bool ReadOnly);
    PathsToFolders();
};

#endif // PATHS_TO_FOLDERS_H
