#include "files.h"

#include <filesystem>
#include <spdlog/spdlog.h>

#include "../utils.h"

Files::Files(const std::string &archiveTempPath) : archiveTempPath_(archiveTempPath)
{
}

Files::~Files()
{
    // Always remove the staged root-owned temp on destruction so it doesn't
    // persist past a single install step (success or failure).
    if (!archiveTempPath_.empty()) {
        std::error_code ec;
        std::filesystem::remove(archiveTempPath_, ec);
    }
}

int Files::executeStep()
{
    const std::string installPath = WS_MAC_APP_DIR;

    // The installer should have removed any existing app instance by this point,
    // but we'll doublecheck just to be sure.
    std::error_code ec;
    if (std::filesystem::exists(installPath, ec)) {
        spdlog::info("Files: install path already exists");
        std::filesystem::remove_all(installPath, ec);
        if (ec) {
            spdlog::error("Files: filesystem::remove_all failed: {}", ec.message());
            return -1;
        }
    }

    std::error_code mkEc;
    if (!std::filesystem::create_directory(installPath, mkEc) || mkEc) {
        spdlog::error("Files: failed to create the app bundle folder: {}", mkEc.message());
        return -1;
    }

    auto status = Utils::executeCommand("tar", {"-xovf", archiveTempPath_.c_str(), "-C", installPath.c_str()}, &lastError_);
    if (status != 0) {
        spdlog::error("Files: failed to untar the app archive: {}", lastError_);
        return -1;
    }

    Utils::executeCommand("xattr", {"-r", "-d", "com.apple.quarantine", installPath.c_str()});

    return 1;
}
