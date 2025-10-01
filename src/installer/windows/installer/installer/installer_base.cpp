#include "installer_base.h"

InstallerBase::InstallerBase()
{
}

InstallerBase::~InstallerBase()
{
    waitForCompletion();
}

void InstallerBase::start()
{
    isCanceled_ = false;
    startImpl();

    extract_thread = std::thread([this] { executionImpl(); });
}

void InstallerBase::cancel()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isCanceled_ = true;
    }
    waitForCompletion();
}

void InstallerBase::waitForCompletion()
{
    if (extract_thread.joinable()) {
        extract_thread.join();
    }
}

void InstallerBase::launchApp()
{
    launchAppImpl();
}
