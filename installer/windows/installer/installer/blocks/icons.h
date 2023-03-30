#pragma once

#include <string>
#include <ShlObj.h>

#include "../iinstall_block.h"
#include "../../../utils/paths_to_folders.h"
#include "../../../utils/redirection.h"

using namespace std;

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

    void createShortcut(const wstring link, const wstring target, const wstring params, const wstring workingDir, const wstring icon, const int idx);
};