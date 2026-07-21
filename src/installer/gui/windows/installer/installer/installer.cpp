#include "installer.h"

#include <spdlog/spdlog.h>

#include "settings.h"
#include "ShellExecuteAsUser.h"

#include "blocks/files.h"
#include "blocks/icons.h"
#include "blocks/install_openvpn_dco.h"
#include "blocks/install_tap_windows6.h"
#include "blocks/install_splittunnel.h"
#include "blocks/service.h"
#include "blocks/uninstall_info.h"
#include "blocks/uninstallprev.h"

#include "../../utils/applicationinfo.h"
#include "../../utils/path.h"
#include "wsscopeguard.h"

using namespace std;

Installer::Installer() : InstallerBase(), state_(wsl::STATE_INIT), progress_(0), error_(wsl::ERROR_OTHER)
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
    blocks_.push_back(new Service(10));
    blocks_.push_back(new InstallOpenVPNDCO(10));
    blocks_.push_back(new InstallTapWindows6(10));
    blocks_.push_back(new InstallSplitTunnel(10));
    blocks_.push_back(new UninstallInfo(5));
    blocks_.push_back(new Icons(Settings::instance().getCreateShortcut(), 5));

    for (auto block : blocks_)
        totalWork_ += block->getWeight();
}

void Installer::executionImpl()
{
    // Catch-all: executionImpl runs on its own std::thread (see InstallerBase::start), so an
    // escaping exception would land in std::terminate and kill the installer without any
    // error state or UI feedback.  Convert it to a logged, reportable error instead.
    try {
        executeBlocks();
    }
    catch (const std::exception &ex) {
        spdlog::error("Installation failed with an unhandled exception: {}", ex.what());
        error_ = wsl::ERROR_INTERNAL;
        state_ = wsl::STATE_ERROR;
        callback_();
    }
    catch (...) {
        spdlog::error("Installation failed with an unhandled exception of unknown type");
        error_ = wsl::ERROR_INTERNAL;
        state_ = wsl::STATE_ERROR;
        callback_();
    }
}

void Installer::executeBlocks()
{
    double completedWeight = 0;

    state_ = wsl::STATE_EXTRACTING;
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
                    state_ = wsl::STATE_CANCELED;
                    progress_ = 0;
                    return;
                }
            }

            int progressOfBlock = block->executeStep();

            // block is finished?
            if (progressOfBlock >= 100 || (progressOfBlock < 0 && !block->isCritical())) {
                completedWeight += block->getWeight();
                progress_ = (int)(100 * completedWeight / totalWork_);
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
                progress_ = (int)(100 * completedWeight / totalWork_);
                error_ = static_cast<wsl::INSTALLER_ERROR>(-progressOfBlock);
                state_ = wsl::STATE_ERROR;
                callback_();
                spdlog::error(L"{}", block->getLastError());
                return;
            } else {
                progress_ = (int)(100 * (completedWeight + progressOfBlock * block->getWeight() / 100.0) / totalWork_);
                callback_();
            }
        }
    }

    // Delete blocks to catch any memory access/corruption errors now rather than when the program exits.
    deleteBlocks();
    exitGuard.dismiss();

    state_ = wsl::STATE_FINISHED;
    progress_ = 100;
    callback_();
}

void Installer::launchAppImpl()
{
    try {
        if (Settings::instance().getAutoStart()) {
            spdlog::info(L"Launching Windscribe app");
            const wstring app = Path::append(Settings::instance().getPath(), ApplicationInfo::appExeName());
            wsl::RunDeElevated(app);
        } else {
            spdlog::info(L"Skipped launching Windscribe app");
        }
    }
    catch (system_error& ex) {
        spdlog::error("Failed to launch Windscribe app: {}", ex.what());
    }

    state_ = wsl::STATE_LAUNCHED;
    callback_();
}

wsl::INSTALLER_STATE Installer::state()
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

wsl::INSTALLER_ERROR Installer::lastError()
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
