#pragma once

#include "helper_base.h"

// Helper commands
class HelperInstaller : public HelperBase
{
public:
    explicit HelperInstaller(std::unique_ptr<IHelperBackend> backend);

    bool removeOldInstall(const std::string &path);
    bool setInstallerPaths(const std::wstring &archivePath, const std::wstring &installPath);
    bool executeFilesStep();
    bool createCliSymlink();

private:
    bool deserializeSuccess(const std::string &data);
};
