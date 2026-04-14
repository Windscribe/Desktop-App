#include "helper_installer.h"

HelperInstaller::HelperInstaller(std::unique_ptr<IHelperBackend> backend) : HelperBase(std::move(backend), spdlog::default_logger_raw())
{
}

bool HelperInstaller::removeOldInstall(const std::string &path)
{
    auto result = sendCommand(HelperCommand::removeOldInstall, path);
    return deserializeSuccess(result);
}

bool HelperInstaller::setInstallerPaths(const std::wstring &archivePath, const std::wstring &installPath)
{
    auto result = sendCommand(HelperCommand::setInstallerPaths, archivePath, installPath);
    return deserializeSuccess(result);
}

bool HelperInstaller::executeFilesStep()
{
    auto result = sendCommand(HelperCommand::executeFilesStep);
    return deserializeSuccess(result);
}

bool HelperInstaller::createCliSymlink()
{
    auto result = sendCommand(HelperCommand::createCliSymlink, getuid());
    return deserializeSuccess(result);
}

bool HelperInstaller::deserializeSuccess(const std::string &data)
{
    bool success;
    deserializeAnswer(data, success);
    return success;
}

