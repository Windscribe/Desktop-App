#ifndef INSTALLER_H
#define INSTALLER_H

#include <list>
#include <string>

#include "installer_base.h"
#include "iinstall_block.h"

class Installer : public InstallerBase
{
protected:
    void startImpl(HWND hwnd, const Settings &settings) override;
    void executionImpl() override;
    void runLauncherImpl() override;

    std::wstring installPath_;
    std::list<IInstallBlock *> blocks_;
    int totalWork_;

public:
    Installer(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState);
    ~Installer() override;
};


#endif // INSTALLER_H
