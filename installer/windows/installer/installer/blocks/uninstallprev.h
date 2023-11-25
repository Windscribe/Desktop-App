#pragma once

#include "../iinstall_block.h"

class UninstallPrev : public IInstallBlock
{
public:
    virtual int executeStep();
    UninstallPrev(bool isFactoryReset, double weight);

private:
    int state_;
    bool isFactoryReset_;

    std::wstring getUninstallString();
    bool uninstallOldVersion(const std::wstring &uninstallString);
    std::wstring removeQuotes(const std::wstring &str);
    void doFactoryReset();
    void stopService();
};
