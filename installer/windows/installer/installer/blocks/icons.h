#ifndef ICONS_H
#define ICONS_H

#include <string.h>
#include <ShlObj.h>

#include "../iinstall_block.h"
#include "../../../utils/paths_to_folders.h"
#include "../../../utils/redirection.h"


//1% - Creating of the Shortcuts
class Icons : public IInstallBlock
{
public:
	Icons(bool isCreateShortcut, double weight);
	~Icons();

	virtual int executeStep();

 private:
    const bool isCreateShortcut_;
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
