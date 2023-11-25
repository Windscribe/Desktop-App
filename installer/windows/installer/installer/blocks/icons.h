#pragma once

#include "../iinstall_block.h"

//1% - Creating of the Shortcuts
class Icons : public IInstallBlock
{
public:
    Icons(bool isCreateShortcut, double weight);
    ~Icons();

    virtual int executeStep();

private:
    const bool isCreateShortcut_;

    void createShortcut(const std::wstring link, const std::wstring target, const std::wstring params,
                        const std::wstring workingDir, const std::wstring icon, const int idx);
};