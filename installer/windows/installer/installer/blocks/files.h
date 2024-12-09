#pragma once

#include <string>

#include "../iinstall_block.h"

class Files : public IInstallBlock
{
public:
    Files(double weight);
    virtual ~Files();

    virtual int executeStep();

private:
    std::wstring installPath_;

    int moveFiles();
    bool copyLibs();
    void logCustomFolderContents(const std::wstring &folder);
};
