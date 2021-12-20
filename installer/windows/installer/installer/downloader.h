#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "installer_base.h"

class Downloader : public InstallerBase
{
protected:
    void startImpl(HWND hwnd, const Settings &settings) override;
    void executionImpl() override;
    void runLauncherImpl() override;

    std::wstring tempInstallerPath_;
    std::wstring legacyInstallerUrl_;

public:
    Downloader(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState);
    ~Downloader() override;
};

#endif // DOWNLOADER_H
