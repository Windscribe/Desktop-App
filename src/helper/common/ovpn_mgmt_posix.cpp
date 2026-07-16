#include "ovpn_mgmt_posix.h"

#include <cerrno>
#include <pwd.h>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <spdlog/spdlog.h>

#include "io_posix.h"
#include "validation_posix.h"

namespace Utils {
int executeCommand(const std::string &cmd,
                   const std::vector<std::string> &args = std::vector<std::string>(),
                   std::string *pOutputStr = nullptr, bool appendFromStdErr = true);
std::vector<std::string> getOpenVpnExeNames();
}

namespace OvpnMgmt {

bool waitAndRelaxPerms(uid_t clientUid, pid_t ovpnPid)
{
    // ~10s window: the socket binds in milliseconds, but on a slow/busy machine the OpenVPN process
    // can take seconds just to start. The engine blocks on this IPC reply with no timeout, so a
    // longer wait is safe and is far better than failing a connect that would otherwise succeed.
    for (int i = 0; i < 200; ++i) {   // ~10s
        struct stat st;
        if (stat(WS_OVPN_MGMT_SOCKET, &st) == 0 && S_ISSOCK(st.st_mode)) {
            // chown before chmod: narrowing to 0600 first would briefly lock the client out while the
            // socket is still root-owned.
            if (chown(WS_OVPN_MGMT_SOCKET, clientUid, (gid_t)-1) != 0) {
                spdlog::error("Could not chown OpenVPN management socket: {}", IO::strerror(errno));
                return false;
            }
            if (chmod(WS_OVPN_MGMT_SOCKET, 0600) != 0) {
                spdlog::error("Could not chmod OpenVPN management socket: {}", IO::strerror(errno));
                return false;
            }
            return true;
        }
        // Bail out early if OpenVPN has already exited (bad config, crash): a dead process will never
        // bind the socket. The helper runs as root, so kill(pid, 0) is a valid liveness probe
        // regardless of the daemon being reparented to init/launchd.
        if (ovpnPid > 0 && kill(ovpnPid, 0) != 0 && errno == ESRCH) {
            spdlog::error("OpenVPN (pid {}) exited before binding its management socket", ovpnPid);
            return false;
        }
        usleep(50 * 1000);
    }
    spdlog::error("OpenVPN management socket did not appear within timeout");
    return false;
}

void killOpenVPN()
{
    const std::vector<std::string> exes = Utils::getOpenVpnExeNames();

    // pkill only *sends* the signal; it does not wait for the process to exit. OpenVPN unlinks its
    // management socket by name during shutdown so a still-dying instance can delete the socket
    // a freshly spawned instance just bound at the same fixed path. We therefore send SIGTERM,
    // wait for the process to actually go away, and escalate to SIGKILL if it doesn't.
    for (const auto &exe : exes) {
        if (exe.empty()) {
            continue;
        }
        // Using the common character-class ("bracket") trick to prevent matching on the wrapper shell.
        const std::string pattern = "[" + exe.substr(0, 1) + "]" + exe.substr(1);

        auto waitGone = [&pattern](int iterations) {
            for (int i = 0; i < iterations; ++i) {
                if (Utils::executeCommand("pkill", {"-0", "-f", pattern.c_str()}) != 0) {
                    return true;
                }
                usleep(50 * 1000);
            }
            return false;
        };

        Utils::executeCommand("pkill", {"-f", pattern.c_str()});
        if (waitGone(100)) {   // ~5s grace period
            continue;
        }

        spdlog::warn("killOpenVPN: OpenVPN binary {} did not exit after SIGTERM; sending SIGKILL", exe);
        Utils::executeCommand("pkill", {"-KILL", "-f", pattern.c_str()});
        if (!waitGone(20)) {   // ~1s to confirm reap
            spdlog::error("killOpenVPN: OpenVPN binary {} still present after SIGKILL", exe);
        }
    }
}

bool mgmtClientUserForUid(uid_t uid, std::string &out)
{
    out.clear();
    const struct passwd *pw = getpwuid(uid);
    if (!pw) {
        spdlog::warn("mgmtClientUserForUid: getpwuid could not resolve the username for uid {}; management-client-user omitted", uid);
        return false;
    }
    if (!Validation::isValidUsername(pw->pw_name)) {
        spdlog::warn("mgmtClientUserForUid: the username for uid {} is invalid; management-client-user omitted", uid);
        return false;
    }
    out = pw->pw_name;
    return true;
}

} // namespace OvpnMgmt
