#ifndef ICONS_H
#define ICONS_H

#include "../iinstall_block.h"
//#include "../../Utils/logger.h"
#include <string.h>
#include <ShlObj.h>
#include "../../../Utils/paths_to_folders.h"
#include "../../../Utils/redirection.h"


//1% - Creating of the Shortcuts
class Icons : public IInstallBlock
{
public:
	Icons(const std::wstring &installPath, bool isCreateShortcut, double weight);
	~Icons();

	virtual int executeStep();

 private:
    std::wstring installPath_;
    bool isCreateShortcut_;
    std::wstring uninstallExeFilename_;
    PathsToFolders pathsToFolders_;
    Redirection redirection;
 
    enum TMakeDirFlags {mdNoUninstall, mdAlwaysUninstall, mdDeleteAfterInstall,mdNotifyChange};

    bool MakeDir(const bool DisableFsRedir,std::wstring Dir, const TMakeDirFlags Flags);

    void CreateAnIcon(std::wstring Name, const std::wstring Description, const std::wstring Path,const std::wstring Parameters,
		std::wstring WorkingDir, std::wstring IconFilename, const int IconIndex,const int ShowCmd,
         const unsigned short HotKey, bool FolderShortcut);
    void DeleteFolderShortcut(const std::wstring Dir);
	std::wstring CreateShellLink(const std::wstring Filename, const std::wstring Description, const std::wstring ShortcutTo, const std::wstring Parameters,
      const std::wstring WorkingDir, const std::wstring IconFileName, const int IconIndex, const int ShowCmd,
      const WORD HotKey, bool FolderShortcut);
    void AssignWorkingDir(IShellLink *pSL, const std::wstring WorkingDir);

};

#endif // ICONS_H
