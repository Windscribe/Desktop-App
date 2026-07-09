#include "nftablescontroller.h"

#include <nftables/libnftables.h>
#include <spdlog/spdlog.h>

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

bool NftablesController::run(const std::string &cmds, bool dryRun, std::string *errOut, bool logOnError)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // A fresh context per call. libnftables caches the ruleset in the nft_ctx; a context reused across
    // transactions in this long-lived helper goes stale after the first commit and then reports EBUSY
    // ("Device or resource busy") on the next `add table`. A new context each time reads live kernel
    // state, so every transaction starts clean — the same way each `nft` CLI invocation does.
    nft_ctx *ctx = nft_ctx_new(NFT_CTX_DEFAULT);
    if (ctx == nullptr) {
        spdlog::error("NftablesController::run: nft_ctx_new failed");
        if (errOut != nullptr) {
            *errOut = "nft_ctx_new failed";
        }
        return false;
    }
    // Capture nft's output/diagnostics into internal buffers instead of the helper's fds.
    nft_ctx_buffer_output(ctx);
    nft_ctx_buffer_error(ctx);

    // dry-run validates the ruleset against the kernel without committing it.
    nft_ctx_set_dry_run(ctx, dryRun);
    const int rc = nft_run_cmd_from_buffer(ctx, cmds.c_str());

    bool ok = true;
    if (rc != 0) {
        const char *err = nft_ctx_get_error_buffer(ctx);
        const std::string errStr = (err != nullptr) ? err : std::string();
        if (logOnError) {
            spdlog::error("nft {} failed (rc {}): {}", dryRun ? "validation" : "command", rc, errStr);
        }
        if (errOut != nullptr) {
            *errOut = errStr;
        }
        ok = false;
    }

    nft_ctx_free(ctx);
    return ok;
}
