#include "files.h"

#include <codecvt>
#include <filesystem>
#include <locale>
#include <spdlog/spdlog.h>

#include "../utils.h"

Files::Files(const std::wstring &archivePath, const std::wstring &installPath) : archivePath_(archivePath), installPath_(installPath)
{
}

Files::~Files()
{
}

int Files::executeStep()
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string archivePath = converter.to_bytes(archivePath_);
    std::string installPath = converter.to_bytes(installPath_);
#pragma clang diagnostic pop

    // The installer should have removed any existing Windscribe app instance by this point,
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

    auto status = Utils::executeCommand("mkdir", {installPath.c_str()}, &lastError_);
    if (status != 0) {
        spdlog::error("Files: failed to create the app bundle folder: {}", lastError_);
        return -1;
    }

    status = Utils::executeCommand("tar", {"-xovf", archivePath.c_str(), "-C", installPath.c_str()}, &lastError_);
    if (status != 0) {
        spdlog::error("Files: failed to untar the app archive: {}", lastError_);
        return -1;
    }

    Utils::executeCommand("xattr", {"-r", "-d", "com.apple.quarantine", installPath.c_str()});

    return 1;
}
