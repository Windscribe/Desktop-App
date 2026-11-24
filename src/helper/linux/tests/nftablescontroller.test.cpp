#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <nftables/libnftables.h>
#include <sched.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../nftables/nftablescontroller.h"

namespace
{

constexpr int kSkipped = 77;
int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        fprintf(stderr, "FAILED line %d: %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

std::string enableRules()
{
    return nft::addTable() + nft::ensureChain("input", kInputChainSpec) + nft::ensureChain("output", kOutputChainSpec)
           + nft::rule("output", "oifname \"lo\" accept");
}

std::string disableRules()
{
    return nft::addTable() + nft::deleteChain("input") + nft::deleteChain("output");
}

bool runOnNewContext(const std::string &cmds)
{
    nft_ctx *ctx = nft_ctx_new(NFT_CTX_DEFAULT);
    if (ctx == nullptr) {
        return false;
    }
    nft_ctx_buffer_output(ctx);
    nft_ctx_buffer_error(ctx);
    const bool ok = nft_run_cmd_from_buffer(ctx, cmds.c_str()) == 0;
    nft_ctx_free(ctx);
    return ok;
}

bool canUseNftables()
{
    return runOnNewContext("add table inet nftprobe\ndelete table inet nftprobe\n");
}

int countNetfilterSockets(bool requireCloexec)
{
    int count = 0;
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        return -1;
    }
    while (const dirent *ent = readdir(dir)) {
        const int fd = atoi(ent->d_name);
        int protocol = 0;
        socklen_t len = sizeof(protocol);
        if (fd <= 2 || fd == dirfd(dir) || getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &protocol, &len) != 0
            || protocol != NETLINK_NETFILTER) {
            continue;
        }
        int domain = 0;
        len = sizeof(domain);
        if (getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &domain, &len) != 0 || domain != AF_NETLINK) {
            continue;
        }
        ++count;
        VERIFY(!requireCloexec || (fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0);
    }
    closedir(dir);
    return count;
}

int outputChainState()
{
    std::string err;
    const int state = NftablesController::instance().chainExists("output", &err);
    if (state < 0) {
        fprintf(stderr, "chainExists error: %s\n", err.c_str());
    }
    return state;
}

void testOnOffCyclesOnOneContext()
{
    auto &nft = NftablesController::instance();
    for (int i = 0; i < 3; ++i) {
        VERIFY(nft.run(enableRules()));
        VERIFY(outputChainState() == 1);
        VERIFY(nft.run(disableRules()));
        VERIFY(outputChainState() == 0);
    }
}

void testFailureThenSuccess()
{
    auto &nft = NftablesController::instance();
    std::string err;
    VERIFY(!nft.run(nft::addTable() + "delete chain " + kNftTable + " nosuch\n", false, &err, false));
    VERIFY(!err.empty());
    VERIFY(nft.run(enableRules()));
    VERIFY(outputChainState() == 1);
    VERIFY(nft.run(disableRules()));
}

void testNetfilterSocketsAreCloseOnExec()
{
    const int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_NETFILTER);
    VERIFY(fd >= 0);
    VERIFY(NftablesController::instance().run(enableRules()));
    VERIFY(fd >= 0 && (fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0);
    VERIFY(countNetfilterSockets(true) >= 1);
    if (fd >= 0) {
        close(fd);
    }
}

void testChildDoesNotInheritNetfilterSockets()
{
    const int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_NETFILTER);
    VERIFY(fd >= 0);
    VERIFY(NftablesController::instance().run(enableRules()));
    const pid_t pid = fork();
    VERIFY(pid >= 0);
    if (pid == 0) {
        execl("/proc/self/exe", "nftablescontroller.test", "--count-netfilter-sockets", static_cast<char *>(nullptr));
        _exit(127);
    }
    int status = 0;
    VERIFY(waitpid(pid, &status, 0) == pid);
    VERIFY(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    if (fd >= 0) {
        close(fd);
    }
}

void testReusedContextAfterExternalChanges()
{
    auto &nft = NftablesController::instance();
    VERIFY(nft.run(enableRules()));
    VERIFY(nft.run(disableRules()));
    VERIFY(outputChainState() == 0);
    VERIFY(runOnNewContext(enableRules()));
    VERIFY(outputChainState() == 1);
    VERIFY(runOnNewContext("delete table " + kNftTable + "\n"));
    VERIFY(outputChainState() == 0);
    VERIFY(nft.run(enableRules()));
    VERIFY(outputChainState() == 1);
    VERIFY(nft.run(disableRules()));
}

void testChainExistsMatchesKernelState()
{
    VERIFY(runOnNewContext("flush ruleset\n"));
    VERIFY(outputChainState() == 0);
    VERIFY(runOnNewContext(nft::addTable()));
    VERIFY(outputChainState() == 0);
    VERIFY(runOnNewContext(nft::ensureChain("input", kInputChainSpec)));
    VERIFY(outputChainState() == 0);
    VERIFY(runOnNewContext("add table ip " WS_PRODUCT_NAME_LOWER "\nadd chain ip " WS_PRODUCT_NAME_LOWER " output\n"));
    VERIFY(outputChainState() == 0);
    VERIFY(runOnNewContext(nft::ensureChain("output", kOutputChainSpec)));
    VERIFY(outputChainState() == 1);
    VERIFY(runOnNewContext("flush ruleset\n"));
    VERIFY(outputChainState() == 0);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--count-netfilter-sockets") {
        return countNetfilterSockets(false) == 0 ? 0 : 1;
    }

    if (unshare(CLONE_NEWUSER | CLONE_NEWNET) != 0 || !canUseNftables()) {
        printf("SKIPPED: cannot use nftables in a user and network namespace\n");
        return kSkipped;
    }

    testOnOffCyclesOnOneContext();
    testFailureThenSuccess();
    testNetfilterSocketsAreCloseOnExec();
    testChildDoesNotInheritNetfilterSockets();
    testReusedContextAfterExternalChanges();
    testChainExistsMatchesKernelState();

    if (g_failures == 0) {
        printf("All nftables controller tests passed\n");
    }
    return g_failures ? 1 : 0;
}
