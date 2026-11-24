#include "nftablescontroller.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netlink.h>
#include <nftables/libnftables.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "utils/log/debug_trace.h"

namespace {

constexpr size_t kMaxBufferedBytes = 1024 * 1024;
constexpr unsigned short kNewChainType = (NFNL_SUBSYS_NFTABLES << 8) | NFT_MSG_NEWCHAIN;

bool appendStringAttr(char *buf, size_t size, size_t &len, unsigned short type, const char *value)
{
    const size_t payload = std::strlen(value) + 1;
    if (len + NLA_ALIGN(NLA_HDRLEN + payload) > size) {
        return false;
    }
    nlattr *attr = reinterpret_cast<nlattr *>(buf + len);
    attr->nla_type = type;
    attr->nla_len = static_cast<unsigned short>(NLA_HDRLEN + payload);
    std::memcpy(buf + len + NLA_HDRLEN, value, payload);
    len += NLA_ALIGN(attr->nla_len);
    return true;
}

bool setNetfilterSocketsCloexec()
{
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        return false;
    }
    bool ok = true;
    const int dirFd = dirfd(dir);
    while (const dirent *ent = readdir(dir)) {
        char *end = nullptr;
        const long fd = std::strtol(ent->d_name, &end, 10);
        if (end == ent->d_name || *end != '\0' || fd == dirFd) {
            continue;
        }
        int domain = 0;
        socklen_t len = sizeof(domain);
        if (getsockopt(static_cast<int>(fd), SOL_SOCKET, SO_DOMAIN, &domain, &len) != 0 || domain != AF_NETLINK) {
            continue;
        }
        int protocol = 0;
        len = sizeof(protocol);
        if (getsockopt(static_cast<int>(fd), SOL_SOCKET, SO_PROTOCOL, &protocol, &len) != 0 || protocol != NETLINK_NETFILTER) {
            continue;
        }
        const int flags = fcntl(static_cast<int>(fd), F_GETFD);
        if (flags < 0) {
            continue;
        }
        if ((flags & FD_CLOEXEC) == 0 && fcntl(static_cast<int>(fd), F_SETFD, flags | FD_CLOEXEC) != 0) {
            ok = false;
        }
    }
    closedir(dir);
    return ok;
}

}  // namespace

NftablesController::NftablesController()
{
}

NftablesController::~NftablesController()
{
}

NftablesController &NftablesController::instance()
{
    static NftablesController inst;
    return inst;
}

void NftablesController::discardContext()
{
    if (ctx_ != nullptr) {
        nft_ctx_free(ctx_);
        ctx_ = nullptr;
    }
}

void NftablesController::closeQuerySocket()
{
    if (querySocket_ >= 0) {
        close(querySocket_);
        querySocket_ = -1;
    }
}

int NftablesController::chainExists(const char *chain, std::string *errOut)
{
    WS_DEBUG_TRACE_SCOPE("NftablesController::chainExists");
    std::lock_guard<std::mutex> lock(mutex_);

    const auto fail = [&](const char *what, int error) {
        closeQuerySocket();
        if (errOut != nullptr) {
            *errOut = std::string(what) + ": " + std::strerror(error);
        }
        return -1;
    };

    if (querySocket_ < 0) {
        querySocket_ = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_NETFILTER);
        if (querySocket_ < 0) {
            return fail("socket", errno);
        }
        const timeval timeout = {2, 0};
        if (setsockopt(querySocket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
            return fail("setsockopt", errno);
        }
    }

    alignas(nlmsghdr) char request[512] = {};
    nlmsghdr *nh = reinterpret_cast<nlmsghdr *>(request);
    nh->nlmsg_type = (NFNL_SUBSYS_NFTABLES << 8) | NFT_MSG_GETCHAIN;
    nh->nlmsg_flags = NLM_F_REQUEST;
    nh->nlmsg_seq = ++querySeq_;
    nfgenmsg *gen = static_cast<nfgenmsg *>(NLMSG_DATA(nh));
    gen->nfgen_family = NFPROTO_INET;
    gen->version = NFNETLINK_V0;
    gen->res_id = 0;
    size_t len = NLMSG_SPACE(sizeof(nfgenmsg));
    if (!appendStringAttr(request, sizeof(request), len, NFTA_CHAIN_TABLE, WS_PRODUCT_NAME_LOWER)
        || !appendStringAttr(request, sizeof(request), len, NFTA_CHAIN_NAME, chain)) {
        return fail("request", ENAMETOOLONG);
    }
    nh->nlmsg_len = static_cast<unsigned int>(len);

    sockaddr_nl kernel = {};
    kernel.nl_family = AF_NETLINK;
    if (sendto(querySocket_, request, len, 0, reinterpret_cast<sockaddr *>(&kernel), sizeof(kernel)) != static_cast<ssize_t>(len)) {
        return fail("sendto", errno);
    }

    for (;;) {
        alignas(nlmsghdr) char reply[8192];
        ssize_t received = recv(querySocket_, reply, sizeof(reply), MSG_TRUNC);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return fail("recv", errno);
        }
        int remaining = static_cast<int>(std::min(static_cast<size_t>(received), sizeof(reply)));
        for (const nlmsghdr *msg = reinterpret_cast<const nlmsghdr *>(reply); NLMSG_OK(msg, remaining);
             msg = NLMSG_NEXT(msg, remaining)) {
            if (msg->nlmsg_seq != querySeq_) {
                continue;
            }
            if (msg->nlmsg_type == kNewChainType) {
                return 1;
            }
            if (msg->nlmsg_type == NLMSG_ERROR) {
                if (msg->nlmsg_len < NLMSG_LENGTH(sizeof(nlmsgerr))) {
                    return fail("reply", EBADMSG);
                }
                const int error = static_cast<const nlmsgerr *>(NLMSG_DATA(msg))->error;
                if (error == -ENOENT || error == -EPERM) {
                    return 0;
                }
                return fail("reply", error < 0 ? -error : EBADMSG);
            }
        }
    }
}

bool NftablesController::runOnce(const std::string &cmds, bool dryRun, std::string &err, int &rc)
{
    if (ctx_ == nullptr) {
        ctx_ = nft_ctx_new(NFT_CTX_DEFAULT);
        if (ctx_ == nullptr) {
            spdlog::error("NftablesController::run: nft_ctx_new failed");
            err = "nft_ctx_new failed";
            rc = -1;
            return false;
        }
        nft_ctx_buffer_output(ctx_);
        nft_ctx_buffer_error(ctx_);
    }

    nft_ctx_set_dry_run(ctx_, dryRun);
    rc = nft_run_cmd_from_buffer(ctx_, cmds.c_str());

    const char *errBuf = nft_ctx_get_error_buffer(ctx_);
    err = (errBuf != nullptr) ? errBuf : std::string();
    if (rc != 0) {
        return false;
    }
    const char *outBuf = nft_ctx_get_output_buffer(ctx_);
    const bool oversized = err.size() > kMaxBufferedBytes || (outBuf != nullptr && std::strlen(outBuf) > kMaxBufferedBytes);

    const bool rebuffered = nft_ctx_unbuffer_output(ctx_) == 0 && nft_ctx_unbuffer_error(ctx_) == 0
                            && nft_ctx_buffer_output(ctx_) == 0 && nft_ctx_buffer_error(ctx_) == 0;
    if (!rebuffered || oversized || !setNetfilterSocketsCloexec()) {
        discardContext();
    }
    return true;
}

bool NftablesController::run(const std::string &cmds, bool dryRun, std::string *errOut, bool logOnError)
{
    WS_DEBUG_TRACE_SCOPE("NftablesController::run");
    std::lock_guard<std::mutex> lock(mutex_);

    const char *what = dryRun ? "validation" : "command";
    const bool reused = ctx_ != nullptr;
    std::string err;
    int rc = 0;
    bool ok = runOnce(cmds, dryRun, err, rc);
    if (!ok && reused) {
        if (logOnError) {
            spdlog::debug("nft {} failed on a reused context (rc {}), retrying with a new one: {}", what, rc, err);
        }
        discardContext();
        ok = runOnce(cmds, dryRun, err, rc);
    }

    if (!ok) {
        discardContext();
        if (logOnError) {
            spdlog::error("nft {} failed (rc {}): {}", what, rc, err);
        }
        if (errOut != nullptr) {
            *errOut = err;
        }
    }
    return ok;
}
