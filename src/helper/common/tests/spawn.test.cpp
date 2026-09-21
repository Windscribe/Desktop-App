#include "../spawn_posix.h"

#include <cstdio>
#include <memory>
#include <pwd.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace {
struct passwd account {};
}

extern "C" struct passwd *getpwnam(const char *)
{
    return &account;
}

int main()
{
    std::ostringstream logs;
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "test", std::make_shared<spdlog::sinks::ostream_sink_mt>(logs)));
    Spawn::Options opts;
    opts.runAsUser = "test-user";
    const struct { uid_t uid; gid_t gid; } cases[] = {{0, 1}, {1, 0}, {0, 0}};
    for (const auto &test : cases) {
        account.pw_uid = test.uid;
        account.pw_gid = test.gid;
        logs.str("");
        pid_t pid = -1;
        bool spawned = Spawn::spawnDetached("/usr/bin/true", {}, opts, &pid);
        if (spawned || pid != 0 || logs.str().find("refusing to drop privileges to UID 0 or GID 0") == std::string::npos) {
            std::fprintf(stderr, "UID %u GID %u was not rejected before spawning: %s", test.uid, test.gid, logs.str().c_str());
            return 1;
        }
    }
    return 0;
}
