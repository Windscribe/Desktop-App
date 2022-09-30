#ifndef INSTALLER_H
#define INSTALLER_H

#include <list>
#include <string>

#include "installer_base.h"
#include "iinstall_block.h"

class Installer : public InstallerBase
{
public:
    Installer(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)>& callbackState);
    ~Installer() override;

protected:
    void startImpl() override;
    void executionImpl() override;
    void runLauncherImpl() override;

    std::list<IInstallBlock *> blocks_;
    int totalWork_ = 0;
};


#endif // INSTALLER_H
