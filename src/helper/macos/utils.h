#pragma once

#include <sys/stat.h>

#include <string>
#include <vector>

namespace Utils
{
    // Scope-local umask override. Restores the previous umask in the destructor so an exception during the guarded
    // operation can't leave the process-wide umask in an unexpected state.
    struct UmaskGuard {
        explicit UmaskGuard(mode_t mask) : prev_(umask(mask)) {}
        ~UmaskGuard() { umask(prev_); }
        mode_t prev_;
    };

    // Wiped as one sweep by deleteSelf. From the build, so the client can check the same paths.
    inline constexpr const char *kVendorDir = WS_MAC_VENDOR_DIR;

    // Root-owned location where installerStageAndVerify drops the verified installer.app
    // and where installerCleanupStaged / deleteSelf wipe it from.
    inline constexpr const char *kInstallerStageDir = WS_MAC_VENDOR_DIR "/update";

    // Root-only location where setInstallerPaths copies and verifies the calling installer's bundle
    // before reading the payload out of it. /Library/Application Support is expected to be root:admin
    // 0755 — unlike /Applications — which Server::run() checks rather than assumes.
    inline constexpr const char *kInstallerPayloadDir = WS_MAC_VENDOR_DIR "/payload";

    // Root-only location the payload archive is copied to for the duration of one install.
    inline constexpr const char *kArchiveTempDir = "/private/var/root/.windscribe-installer";

    // The only copies of dns.sh and the bundled binaries the helper runs, from the verified payload.
    // Not the app bundle: the console user can rewrite /Applications. 0755 because the app-user spawns
    // resolve paths through here and the client stats them; what matters is that only root can write.
    inline constexpr const char *kHelperBinDir = WS_MAC_HELPER_BIN_DIR;
    inline constexpr const char *kHelperFrameworksDir = WS_MAC_VENDOR_DIR "/Frameworks";

    // Create kVendorDir root-owned and writable by root alone. A leaf's mode means nothing while its
    // parent can be renamed, so every path that creates one calls this first. Idempotent.
    bool ensureVendorDir();

    // execute cmd with args and return output from stdout and stderror to pOutputStr (if pOutputStr != NULL)
    // When appendFromStdErr is true, stderr is merged into stdout via a subshell redirect, so error
    // lines are interleaved with stdout in arrival order rather than appended after it. Callers must
    // not assume stdout precedes stderr in pOutputStr (e.g. do not treat the first line as data);
    // parse by matching an expected marker/pattern instead.
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr,
                       bool appendFromStdErr = true);

    // find case insensitive sub string in a given substring
    size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos = 0);

    bool isFileExists(const std::string &name);

    // get list of openvpn exe names from package
    std::vector<std::string> getOpenVpnExeNames();

    void createAppUserAndGroup();

    // check if the app has been uninstalled
    bool isAppUninstalled();

    // delete this helper app and the cli symlink
    void deleteSelf();

    // check if string has whitespace
    bool hasWhitespaceInString(const std::string &str);

    // Root-only directory holding everything the helper executes as root.
    std::string getExePath();

    // check if a string is a canonical SCDynamicStore DNS entry of the form
    // (State|Setup):/Network/Service/<id>/DNS, where <id> is alphanumeric/._-
    bool isValidDnsDynamicStoreEntry(const std::string &entry);

    bool isPortListening(unsigned int port, int maxRetries = 10, int delayMs = 100);

    // Apple's link-local peer-to-peer interfaces (AWDL / p2p / llw — AirDrop, AirPlay, Handoff,
    // Continuity). They carry no internet route, so passing them is not a firewall leak. Names are
    // validated (Validation::isValidInterfaceName) before being returned so callers may interpolate
    // them into pf rule text. Shared by the runtime ruleset (FirewallController) and the boot ruleset
    // (FirewallOnBootManager) so the two stay in lockstep.
    std::vector<std::string> getAwdlP2pInterfaces();
};
