#include "files.h"

#include <filesystem>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../utils.h"
#include "../../common/io_posix.h"

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

// Restrict modes (no set-id, no non-owner write) and strip com.apple.quarantine, in one walk. tar
// extracting as root applies the archive's modes verbatim, so otherwise they are whatever the payload
// carried. Symlinks are skipped: chmod would follow them, XATTR_NOFOLLOW would not.
static bool secureExtractedTree(const std::string &root)
{
    bool ok = true;
    ::removexattr(root.c_str(), "com.apple.quarantine", XATTR_NOFOLLOW);

    std::error_code walkEc;
    auto walkEnd = std::filesystem::recursive_directory_iterator();
    for (auto it = std::filesystem::recursive_directory_iterator(root, walkEc);
         !walkEc && it != walkEnd;
         it.increment(walkEc)) {
        const std::string path = it->path().string();
        ::removexattr(path.c_str(), "com.apple.quarantine", XATTR_NOFOLLOW);

        struct stat st;
        if (lstat(path.c_str(), &st) != 0) {
            spdlog::error("Files: lstat(\"{}\") failed: {}", path, IO::strerror(errno));
            ok = false;
            continue;
        }
        if (S_ISLNK(st.st_mode)) {
            continue;
        }
        const mode_t current = st.st_mode & 07777;
        const mode_t safe = current & ~(mode_t)(S_ISUID | S_ISGID | S_IWGRP | S_IWOTH);
        if (safe != current && chmod(path.c_str(), safe) != 0) {
            spdlog::error("Files: could not restrict mode of \"{}\" to {:04o}: {}", path, safe,
                          IO::strerror(errno));
            ok = false;
        }
    }
    if (walkEc) {
        spdlog::error("Files: walk of \"{}\" stopped early: {}", root, walkEc.message());
        ok = false;
    }
    return ok;
}

// A half-populated bin/ or Frameworks/ must not outlive the failed install that produced it: the
// next install wipes the directory before extracting, but nothing else would.
static bool cleanupAndFail(const std::string &dir)
{
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    return false;
}

// Extract selected members of the payload into a root-only directory and leave it root-owned 0755.
// These become the only copies of dns.sh and the bundled executables that the helper ever runs.
bool Files::extractToRootOnlyDir(const std::string &dir, const std::vector<std::string> &members)
{
    // tar with no member arguments extracts the whole archive, so an empty list here would fill the
    // directory with a --strip-components=3 flattening of the entire bundle.
    if (members.empty()) {
        spdlog::error("Files: no archive members given for \"{}\"", dir);
        return false;
    }

    if (!Utils::ensureVendorDir()) {
        return false;
    }

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    {
        Utils::UmaskGuard guard(022);
        std::filesystem::create_directories(dir, ec);
    }
    if (ec) {
        spdlog::error("Files: create_directories(\"{}\") failed: {}", dir, ec.message());
        return cleanupAndFail(dir);
    }
    // Ownership before mode, so a pre-existing directory with another owner is not handed
    // exclusive write to itself by the chmod.
    if (lchown(dir.c_str(), 0, 0) != 0 || chmod(dir.c_str(), 0755) != 0) {
        spdlog::error("Files: could not secure \"{}\": {}", dir, IO::strerror(errno));
        return cleanupAndFail(dir);
    }

    // --strip-components flattens Contents/<subdir>/ so the files land directly in dir.
    std::vector<std::string> args = {"-xof", archiveTempPath_, "-C", dir, "--strip-components=3"};
    args.insert(args.end(), members.begin(), members.end());
    auto status = Utils::executeCommand("tar", args, &lastError_, true);
    if (status != 0) {
        spdlog::error("Files: failed to extract into \"{}\" (exit {}): {}", dir, status, lastError_);
        return cleanupAndFail(dir);
    }

    if (!secureExtractedTree(dir)) {
        spdlog::error("Files: refusing to keep \"{}\" with unrestricted modes", dir);
        return cleanupAndFail(dir);
    }

    return true;
}

int Files::executeStep()
{
    const std::string installPath = WS_MAC_APP_DIR;
    const std::string stagingPath = std::string(Utils::kInstallerPayloadDir) + "/staged.app";

    if (!Utils::ensureVendorDir()) {
        lastError_ = "could not secure the vendor directory";
        return -1;
    }

    // Extract into a root-only directory and rename into place, rather than extracting directly
    // into /Applications. Extracting in place has a window between removing the old bundle and
    // creating the new one in which a symlink can be planted at the install path, and `tar -C`
    // follows it — root writing wherever the attacker points. rename() onto a symlink fails
    // ENOTDIR instead of following, so the worst case here is a failed install.
    std::error_code ec;
    // create_directories() reports no error for a directory that already exists, so a failed wipe
    // would go unnoticed and the new bundle would be extracted on top of whatever survived.
    std::filesystem::remove_all(Utils::kInstallerPayloadDir, ec);
    if (ec) {
        lastError_ = ec.message();
        spdlog::error("Files: failed to clear the staging folder: {}", lastError_);
        return -1;
    }
    {
        Utils::UmaskGuard guard(022);
        std::filesystem::create_directories(stagingPath, ec);
    }
    if (ec) {
        lastError_ = ec.message();
        spdlog::error("Files: failed to create the staging folder: {}", lastError_);
        return -1;
    }
    if (lchown(Utils::kInstallerPayloadDir, 0, 0) != 0 ||
        chmod(Utils::kInstallerPayloadDir, S_IRWXU) != 0) {
        spdlog::error("Files: could not secure the staging folder: {}", IO::strerror(errno));
        return -1;
    }

    // No -v: the verbose file listing is never consumed and would only add noise. stderr is
    // captured so a failure's actual error text reaches the log; extraction writes files to
    // disk and produces no stdout, so draining stderr here cannot deadlock.
    auto status = Utils::executeCommand("tar", {"-xof", archiveTempPath_.c_str(), "-C", stagingPath.c_str()}, &lastError_, true);
    if (status != 0) {
        spdlog::error("Files: failed to untar the app archive (exit code {}): {}", status, lastError_);
        return -1;
    }

    // The dylib names come from the build so the OpenSSL SONAME isn't spelled out here as well; it
    // moves with the pinned port, and reconstructing it from the version would not be reliable.
    std::vector<std::string> frameworkMembers;
    std::istringstream frameworkNames(WS_MAC_HELPER_FRAMEWORK_FILES);
    for (std::string name; frameworkNames >> name; ) {
        frameworkMembers.push_back("./Contents/Frameworks/" + name);
    }

    // Place the root-only copies of everything the helper executes, from the same verified payload
    // — never copied back out of /Applications, which is writable by the console user.
    if (!extractToRootOnlyDir(Utils::kHelperBinDir,
                              {"./Contents/Helpers", "./Contents/Resources/dns.sh"}) ||
        !extractToRootOnlyDir(Utils::kHelperFrameworksDir, frameworkMembers)) {
        return -1;
    }

    // Before the rename, since rename() preserves modes and xattrs. Best effort, unlike the two
    // directories above: the helper executes nothing from /Applications, so a mode left unrestricted
    // here is hygiene rather than integrity and must not fail an otherwise good install.
    if (!secureExtractedTree(stagingPath)) {
        spdlog::warn("Files: could not fully restrict modes in the staged bundle");
    }

    if (std::filesystem::exists(installPath, ec)) {
        spdlog::info("Files: install path already exists");
        std::filesystem::remove_all(installPath, ec);
        if (ec) {
            spdlog::error("Files: filesystem::remove_all failed: {}", ec.message());
            return -1;
        }
    }

    std::filesystem::rename(stagingPath, installPath, ec);
    if (ec) {
        lastError_ = ec.message();
        spdlog::error("Files: failed to move the staged bundle into place: {}", lastError_);
        return -1;
    }

    return 1;
}
