#pragma once

#include <optional>
#include <string>

namespace InstallerVerifier {

// Stages a downloaded update package, verifies the bundled public key against the compile-time
// master-fingerprint anchor, and verifies the package signature using that key. The source path
// is expected to be the package file in a user-writable directory; sibling files (.pub for all
// formats, .asc for .deb, .sig for .pkg.tar.zst) must already be present next to it.
//
// On success, returns the path under /var/lib/windscribe/update/ where the package was staged —
// callers should pass this to install-update instead of the original source path. On failure,
// returns std::nullopt and logs a tagged reason (source-rejected / fingerprint-mismatch /
// signature-invalid / format-unsupported) via spdlog.
std::optional<std::string> stageAndVerify(const std::string &srcPath);

// Removes any state left in /var/lib/windscribe/update/. Safe to call when nothing is staged.
// Invoked from engine init when pendingInstallerCleanup was set by a prior session.
void cleanupStaged();

}  // namespace InstallerVerifier
