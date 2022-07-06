#include "installer.h"
#include <assert.h>
#include "../../utils/logger.h"
#include "blocks/uninstallprev.h"
#include "blocks/files.h"
#include "blocks/uninstall_info.h"
#include "blocks/icons.h"
#include "blocks/service.h"
#include "blocks/install_tap.h"
#include "blocks/install_wintun.h"
#include "blocks/install_splittunnel.h"
#include "blocks/install_authhelper.h"
#include "blocks/ShellExecuteAsUser.h"

using namespace std;

Installer::Installer(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState)
    : InstallerBase(callbackState)
{
}

Installer::~Installer()
{
    for (std::list<IInstallBlock *>::iterator it = blocks_.begin(); it != blocks_.end(); ++it)
    {
        IInstallBlock *install_block = (*it);

        if (install_block != nullptr)
        {
            delete install_block;
        }
    }
}

void Installer::startImpl(HWND hwnd, const Settings &settings)
{
    installPath_ = settings.getPath();
#ifdef _WIN32
    blocks_.push_back(new UninstallPrev(settings.getFactoryReset(), 10));
    blocks_.push_back(new Files(installPath_, 40));
    blocks_.push_back(new Service(installPath_, 5));
    if (settings.getInstallDrivers())
    {
        blocks_.push_back(new InstallTap(installPath_, 10));
        blocks_.push_back(new InstallWinTun(installPath_, 10));
    }
    blocks_.push_back(new InstallSplitTunnel(installPath_, 10, hwnd));
    blocks_.push_back(new UninstallInfo(installPath_, 5));
    blocks_.push_back(new Icons(installPath_, settings.getCreateShortcut(), 5));
    blocks_.push_back(new InstallAuthHelper(installPath_, 5));

    for (auto block : blocks_)
        totalWork_ += block->getWeight();
#endif
}

void Installer::executionImpl()
{
    // try create install directory
    if (SHCreateDirectoryEx(NULL, installPath_.c_str(), nullptr) != ERROR_SUCCESS)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            strLastError_ = L"Can't create install directory";
            callbackState_(0, STATE_FATAL_ERROR);
            return;
        }
    }

    int overallProgress = 0;
    int prevOverallProgress = 0;

    callbackState_(static_cast<unsigned int>(overallProgress), STATE_EXTRACTING);

    std::vector<DWORD> ticks;

    for (std::list<IInstallBlock *>::iterator it = blocks_.begin(); it != blocks_.end(); ++it)
    {
        DWORD initTick = GetTickCount();
        IInstallBlock *block = *it;

        Log::instance().out(L"Installing %ls...", block->getName().c_str());

        while (true)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (isCanceled_)
                {
                    callbackState_(0, STATE_CANCELED);
                    return;
                }
                if (isPaused_)
                {
                    Sleep(1);
                    continue;
                }
            }

            int progressOfBlock = block->executeStep();
            
            // block is finished?
            if (progressOfBlock >= 100 || (progressOfBlock < 0 && !block->isCritical()))
            {
                overallProgress = prevOverallProgress + (int)(100 * block->getWeight() / totalWork_);
                callbackState_(static_cast<unsigned int>(overallProgress), STATE_EXTRACTING);
                ticks.push_back(GetTickCount() - initTick);
                if (progressOfBlock < 0)
                    Log::instance().out(L"Non-critical error installing %ls", block->getName().c_str());
                else
                    Log::instance().out(L"Installed %ls", block->getName().c_str());
                break;
            }
            // error from block?
            else if (progressOfBlock < 0)
            {
                strLastError_ = block->getLastError();
                callbackState_(static_cast<unsigned int>(overallProgress), STATE_FATAL_ERROR);
                Log::instance().out(strLastError_);
                return;
            }
            else
            {
                overallProgress = prevOverallProgress + (int)(progressOfBlock * block->getWeight() / totalWork_);
                callbackState_(static_cast<unsigned int>(overallProgress), STATE_EXTRACTING);
            }
        }
        prevOverallProgress = overallProgress;

    }

    callbackState_(static_cast<unsigned int>(100), STATE_FINISHED);
}

void Installer::runLauncherImpl()
{
 #ifdef _WIN32
    wstring pathLauncher = installPath_ + L"\\WindscribeLauncher.exe";
    Log::instance().out(L"Running launcher: %ls", pathLauncher.c_str());
    ShellExecuteAsUser::shellExecuteFromExplorer(pathLauncher.c_str(), NULL, NULL, NULL, SW_RESTORE);
    //ShellExecute(nullptr, nullptr, pathLauncher.c_str(), nullptr, nullptr, SW_RESTORE);
 #endif
}
