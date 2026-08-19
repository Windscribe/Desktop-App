#pragma once

#include <string>
#include <vector>

namespace Staging
{
    // Copy a single regular file src -> dst without following symlinks at either end, creating dst
    // fresh with mode 0600. Rejects a src that is a symlink or not a regular file (fifo/dir/device)
    // and a dst that already exists. Used to freeze the downloaded DMG into the root-owned stage
    // before it is verified, so a same-user attacker cannot swap or smuggle content through the copy.
    bool freezeRegularFile(const std::string &srcPath, const std::string &dstPath);

    // Run exePath with args as a child, SIGKILLing it after timeoutSecs. Returns the child exit code,
    // -1 on spawn/wait failure, or -2 on timeout. Bounds hdiutil so a wedged attach/detach on a busy
    // volume can't hang the synchronous helper.
    int runWithTimeout(const std::string &exePath, const std::vector<std::string> &args, int timeoutSecs);
}
