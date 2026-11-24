#include "process_command_posix.h"

#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "helper_commands.h"
#include "ovpn_mgmt_posix.h"
#include "ovpn_posix.h"
#include "process_kill_posix.h"
#include "utils/log/debug_trace.h"
#include "utils_posix.h"
#include "validation_posix.h"

std::string executeTaskKill(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("executeTaskKill");
    CmdKillTarget target;
    deserializePars(pars, target);
    bool success = false;

    if (target == kTargetApp) {
        spdlog::info("Killing " WS_PRODUCT_NAME " processes");
        ProcessKill::terminate(WS_APP_EXECUTABLE_NAME);
#ifdef WS_IS_WINDSCRIBE
        ProcessKill::terminate(WS_APP_IDENTIFIER "Engine"); // For older 1.x clients
#endif
        success = true;
    } else if (target == kTargetOpenVpn) {
        spdlog::info("Killing OpenVPN processes");
        OvpnMgmt::killOpenVPN();
#if defined(__linux__)
        OVPN::setUseDco(true);
#endif
        success = true;
    } else if (target == kTargetStunnel) {
        spdlog::info("Killing Stunnel processes");
        ProcessKill::terminate(WS_PRODUCT_NAME_LOWER "wstunnel");
        success = true;
    } else if (target == kTargetWStunnel) {
        spdlog::info("Killing WStunnel processes");
        ProcessKill::terminate(WS_PRODUCT_NAME_LOWER "wstunnel");
        success = true;
    } else if (target == kTargetWireGuard) {
        spdlog::info("Killing WireGuard processes");
        ProcessKill::terminate(WS_PRODUCT_NAME_LOWER "amneziawg");
        success = true;
    } else if (target == kTargetCtrld) {
        spdlog::info("Killing ctrld processes");
        // The next ctrld binds the same 127.0.0.1:53 and exits if a dying instance still holds it.
        success = ProcessKill::terminateAndWait(WS_PRODUCT_NAME_LOWER "ctrld");
    } else {
        spdlog::error("Did not kill processes for type {}", (int)target);
        success = false;
    }
    return serializeResult(success);
}

std::string startCtrld(const std::string &pars)
{
    WS_DEBUG_TRACE_SCOPE("startCtrld");
    std::string upstream1, upstream2;
    std::vector<std::string> domains;
    bool isCreateLog;
    deserializePars(pars, upstream1, upstream2, domains, isCreateLog);

    spdlog::debug("Starting ctrld");

    // Validate URLs, then pass the normalized (canonical) form to ctrld rather than the raw input.
    const std::string normUpstream1 = Validation::normalizeAddress(upstream1);
    const std::string normUpstream2 = Validation::normalizeAddress(upstream2);
    if (normUpstream1.empty() || (!upstream2.empty() && normUpstream2.empty())) {
        spdlog::error("Invalid upstream URL(s)");
        return serializeResult(false);
    }
    for (const auto &domain: domains) {
        if (!Validation::isValidDomain(domain)) {
            spdlog::error("Invalid domain: {}", domain);
            return serializeResult(false);
        }
    }

    std::string ctrldPath;
    if (!Utils::locateHelperExecutable(WS_PRODUCT_NAME_LOWER "ctrld", ctrldPath)) {
        return serializeResult(false);
    }

    std::vector<std::string> args = {"run", "--daemon", "--listen=127.0.0.1:53",
                                     "--primary_upstream=" + normUpstream1};
    if (!upstream2.empty()) {
        args.push_back("--secondary_upstream=" + normUpstream2);
        if (!domains.empty()) {
            std::string domainsStr;
            for (size_t i = 0; i < domains.size(); ++i) {
                if (i > 0) domainsStr += ",";
                domainsStr += domains[i];
            }
            args.push_back("--domains=" + domainsStr);
        }
    }
    if (isCreateLog) {
        args.push_back("--log");
#if defined(__APPLE__)
        args.push_back("/Library/Logs/" WS_MAC_HELPER_BUNDLE_ID "/ctrld.log");
#else
        args.push_back(WS_LINUX_LOG_DIR "/ctrld.log");
#endif
        args.push_back("-vv");
    }

    bool executed = Utils::executeCommand(ctrldPath, args) == 0;
    return serializeResult(executed);
}
