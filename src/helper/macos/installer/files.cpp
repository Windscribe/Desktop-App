#include "files.h"

#include <filesystem>
#include <sys/xattr.h>
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

    std::filesystem::create_directory(installPath, ec);
    if (ec) {
        lastError_ = ec.message();
        spdlog::error("Files: failed to create the app bundle folder: {}", lastError_);
        return -1;
    }

    auto status = Utils::executeCommand("tar", {"-xovf", archiveTempPath_.c_str(), "-C", installPath.c_str()}, &lastError_);
    if (status != 0) {
        spdlog::error("Files: failed to untar the app archive: {}", lastError_);
        return -1;
    }

    // Strip com.apple.quarantine from every entry in the install tree. Equivalent to
    // `xattr -r -d com.apple.quarantine <installPath>` done in-process. XATTR_NOFOLLOW
    // operates on symlinks themselves (not their targets), so symlinked content outside
    // the bundle can't be touched. ENOATTR on entries that never had the attr is fine
    // and is the common case — per-call errors are ignored. A walk error (couldn't
    // descend into a directory) is logged so it doesn't disappear silently.
    ::removexattr(installPath.c_str(), "com.apple.quarantine", XATTR_NOFOLLOW);
    std::error_code walkEc;
    auto walkEnd = std::filesystem::recursive_directory_iterator();
    for (auto it = std::filesystem::recursive_directory_iterator(installPath, walkEc);
         !walkEc && it != walkEnd;
         it.increment(walkEc)) {
        ::removexattr(it->path().c_str(), "com.apple.quarantine", XATTR_NOFOLLOW);
    }
    if (walkEc) {
        spdlog::warn("Files: quarantine strip walk stopped early: {}", walkEc.message());
    }

    return 1;
}
