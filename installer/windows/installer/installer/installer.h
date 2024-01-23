#pragma once

#include <functional>
#include <list>

#include "installer_base.h"
#include "iinstall_block.h"

class Installer : public InstallerBase
{
public:
    Installer();
    ~Installer() override;

    INSTALLER_CURRENT_STATE state();
    INSTALLER_ERROR lastError();
    int progress();
    void setCallback(std::function<void()> func);

protected:
    void startImpl() override;
    void executionImpl() override;
    void launchAppImpl() override;

    std::list<IInstallBlock *> blocks_;
    int totalWork_ = 0;

    std::function<void()> callback_;
    INSTALLER_CURRENT_STATE state_;
    INSTALLER_ERROR error_;
    int progress_;
};
