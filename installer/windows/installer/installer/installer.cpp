#include "installer.h"

#include <spdlog/spdlog.h>

#include "settings.h"
#include "ShellExecuteAsUser.h"

#include "blocks/files.h"
#include "blocks/icons.h"
#include "blocks/install_authhelper.h"
#include "blocks/install_openvpn_dco.h"
#include "blocks/install_splittunnel.h"
#include "blocks/service.h"
#include "blocks/uninstall_info.h"
#include "blocks/uninstallprev.h"

#include "../../utils/applicationinfo.h"
#include "../../utils/path.h"
#include "wsscopeguard.h"

using namespace std;

Installer::Installer() : InstallerBase(), state_(STATE_INIT), progress_(0), error_(ERROR_OTHER)
{
}

Installer::~Installer()
{
    deleteBlocks();
}

void Installer::startImpl()
{
    blocks_.push_back(new UninstallPrev(Settings::instance().getFactoryReset(), 10));
    blocks_.push_back(new Files(40));
    blocks_.push_back(new Service(5));

    // Leaving this option here for now in anticipation of us adding installation of the OpenVPN DCO
    // driver in a future release.
    if (Settings::instance().getInstallDrivers()) {
        blocks_.push_back(new InstallOpenVPNDCO(10));
    }

    blocks_.push_back(new InstallSplitTunnel(10));
    blocks_.push_back(new UninstallInfo(5));
    blocks_.push_back(new Icons(Settings::instance().getCreateShortcut(), 5));
    blocks_.push_back(new InstallAuthHelper(5));

    for (auto block : blocks_)
        totalWork_ += block->getWeight();
}

void Installer::executionImpl()
{
    int overallProgress = 0;
    int prevOverallProgress = 0;

    state_ = STATE_EXTRACTING;
    callback_();

    vector<DWORD> ticks;

    auto exitGuard = wsl::wsScopeGuard([&] {
        deleteBlocks();
    });

    for (list<IInstallBlock *>::iterator it = blocks_.begin(); it != blocks_.end(); ++it) {
        DWORD initTick = GetTickCount();
        IInstallBlock *block = *it;

        spdlog::info(L"Installing {} ...", block->getName());

        while (true) {
            {
                lock_guard<mutex> lock(mutex_);
                if (isCanceled_) {
                    state_ = STATE_CANCELED;
                    progress_ = 0;
                    return;
                }
            }

            int progressOfBlock = block->executeStep();

            // block is finished?
            if (progressOfBlock >= 100 || (progressOfBlock < 0 && !block->isCritical())) {
                overallProgress = prevOverallProgress + (int)(100 * block->getWeight() / totalWork_);
                progress_ = overallProgress;
                callback_();
                ticks.push_back(GetTickCount() - initTick);
                if (progressOfBlock < 0) {
                    spdlog::warn(L"Non-critical error installing {}", block->getName());
                } else {
                    spdlog::info(L"Installed {}", block->getName());
                }
                break;
            }
            // error from block?
            else if (progressOfBlock < 0) {
                progress_ = overallProgress;
                error_ = static_cast<INSTALLER_ERROR>(-progressOfBlock);
                state_ = STATE_ERROR;
                callback_();
                spdlog::error(L"{}", block->getLastError());
                return;
            } else {
                overallProgress = prevOverallProgress + (int)(progressOfBlock * block->getWeight() / totalWork_);
                progress_ = overallProgress;
                callback_();
            }
        }
        prevOverallProgress = overallProgress;

    }

    // Delete blocks to catch any memory access/corruption errors now rather than when the program exits.
    deleteBlocks();
    exitGuard.dismiss();

    state_ = STATE_FINISHED;
    progress_ = 100;
    callback_();
}

void Installer::launchAppImpl()
{
    if (Settings::instance().getAutoStart()) {
        wstring app = Path::append(Settings::instance().getPath(), ApplicationInfo::appExeName());
        spdlog::info(L"Launching Windscribe app");
        ShellExec::executeFromExplorer(app.c_str(), NULL, NULL, NULL, SW_RESTORE);
    } else {
        spdlog::info(L"Skip launching app");
    }
    state_ = STATE_LAUNCHED;
    callback_();
}

INSTALLER_CURRENT_STATE Installer::state()
{
    return state_;
}

int Installer::progress()
{
    return progress_;
}

void Installer::setCallback(function<void()> func)
{
    callback_ = func;
}

INSTALLER_ERROR Installer::lastError()
{
    return error_;
}

void Installer::deleteBlocks()
{
    for (list<IInstallBlock *>::iterator it = blocks_.begin(); it != blocks_.end();) {
        IInstallBlock *install_block = (*it);
        if (install_block != nullptr) {
            spdlog::info(L"Deleting block resource {}", install_block->getName());
            delete install_block;
        }
        it = blocks_.erase(it);
    }
}
