#pragma once

#include <QSettings>

#include "../iinstall_block.h"

//  Register uninstall information so the program can be uninstalled
//  through the Add/Remove Programs Control Panel applet.
class UninstallInfo : public IInstallBlock
{
public:
    UninstallInfo(double weight);
    virtual ~UninstallInfo();

    int executeStep();

    static void addMissingUninstallInfo(const std::wstring &uninstaller);

private:
    void registerUninstallInfo();
};
