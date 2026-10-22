#include "ws_branding.h"
#include "openvpncontroller.h"

#include <cstdint>
#include <codecvt>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <boost/algorithm/string.hpp>

#include "types/global_consts.h"
#include "../utils.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif

#include "../../common/ovpn_directive_whitelist.h"
#include "../../common/ovpn_early_exit_log.h"

namespace {

const char *tcpStateName(DWORD state)
{
    switch (state) {
    case MIB_TCP_STATE_CLOSED:
        return "CLOSED";
    case MIB_TCP_STATE_LISTEN:
        return "LISTEN";
    case MIB_TCP_STATE_SYN_SENT:
        return "SYN_SENT";
    case MIB_TCP_STATE_SYN_RCVD:
        return "SYN_RCVD";
    case MIB_TCP_STATE_ESTAB:
        return "ESTABLISHED";
    case MIB_TCP_STATE_FIN_WAIT1:
        return "FIN_WAIT1";
    case MIB_TCP_STATE_FIN_WAIT2:
        return "FIN_WAIT2";
    case MIB_TCP_STATE_CLOSE_WAIT:
        return "CLOSE_WAIT";
    case MIB_TCP_STATE_CLOSING:
        return "CLOSING";
    case MIB_TCP_STATE_LAST_ACK:
        return "LAST_ACK";
    case MIB_TCP_STATE_TIME_WAIT:
        return "TIME_WAIT";
    case MIB_TCP_STATE_DELETE_TCB:
        return "DELETE_TCB";
    case 0:
        // Not in MIB_TCP_STATE. Windows 11 insider 26340 reports a live loopback listener this way.
        return "UNSPECIFIED";
    default:
        return "UNKNOWN";
    }
}

// MIB_TCP_STATE_LISTEN is accepted on local address alone. Microsoft documents the remote fields as
// meaningless for a listener, and the port's upper 16 bits may be uninitialized, so requiring them
// to be zero can reject a listener this function used to accept.
// Insider build 26340 reports that same listener with dwState 0. Only that undocumented state uses
// the empty-remote heuristic, and only the low 16 bits of the remote port are meaningful.
bool isManagementListener(const MIB_TCPROW2 *entry, DWORD loopbackAddr)
{
    if (entry->dwLocalAddr != loopbackAddr) {
        return false;
    }
    if (entry->dwState == MIB_TCP_STATE_LISTEN) {
        return true;
    }
    return entry->dwState == 0 &&
           entry->dwRemoteAddr == 0 &&
           (entry->dwRemotePort & 0xffffu) == 0;
}

} // namespace

OpenVPNController::OpenVPNController()
{
}

void OpenVPNController::release()
{
    terminateProcess();
    removeAdapter();
}

bool OpenVPNController::createAdapter(bool useDCODriver)
{
    openVpnProcess_.closeHandle();

    // Adapter state does not survive helper restarts, so don't trust adapterCreated_ here: always
    // delete any leftover adapter by name, or the create below can collide with it and OpenVPN may
    // bind an adapter from the wrong driver via dev-node.
    deleteAdapter(true);

    useDCODriver_ = useDCODriver;
    adapterCreated_ = true;

    std::wstringstream stream;
    stream << L"\"" << Utils::getExePath() << L"\\tapctl.exe\"" << L" create --name " << kOpenVPNAdapterIdentifier
           << L" --hwid " << (useDCODriver ? L"ovpn-dco" : L"root\\tap0901");

    spdlog::info(L"createAdapter cmd = {}", stream.str());
    auto result = ExecuteCmd::instance().executeBlockingCmd(stream.str());

    if (result.success) {
        if (result.exitCode != 0) {
            spdlog::error(L"createAdapter cmd failed: {}", result.output);
        } else {
            // Log the output on success too: tapctl renames the adapter with NciSetConnectionName and
            // falls back to netsh if that fails. The fallback warning goes to stderr even when the
            // create succeeds, and without this it would never reach the log.
            spdlog::info(L"createAdapter cmd output: {}", result.output);
        }
    }

    return result.success && result.exitCode == 0;
}

void OpenVPNController::removeAdapter()
{
    msgChannel_.stop();
    // The OpenVPN process has been (or is being) torn down; release the retained handle.
    openVpnProcess_.closeHandle();

    if (adapterCreated_) {
        adapterCreated_ = false;
        deleteAdapter();
    }
}

bool OpenVPNController::terminateProcess()
{
    bool hadProcess = false;
    if (openVpnProcess_.isValid()) {
        hadProcess = true;
        if (!openVpnProcess_.isSignaled()) {
            if (!::TerminateProcess(openVpnProcess_.getHandle(), 1)) {
                spdlog::error("OpenVPNController::terminateProcess TerminateProcess failed: {}", ::GetLastError());
                msgChannel_.stop();
                return false;
            }
            openVpnProcess_.wait(5000);
        }
    }
    msgChannel_.stop();
    openVpnProcess_.closeHandle();
    return hadProcess;
}

void OpenVPNController::deleteAdapter(bool bestEffort)
{
    std::wstringstream stream;
    stream << L"\"" << Utils::getExePath() << L"\\tapctl.exe\"" << L" delete " << kOpenVPNAdapterIdentifier;

    spdlog::info(L"deleteAdapter cmd = {}", stream.str());
    auto result = ExecuteCmd::instance().executeBlockingCmd(stream.str());

    // A best-effort delete usually finds no adapter to remove; that's not worth logging.
    if (result.success && result.exitCode != 0 && !bestEffort) {
        spdlog::error(L"deleteAdapter cmd failed: {}", result.output);
    }
}

ExecuteCmdResult OpenVPNController::runOpenvpn(std::wstring &config, const std::wstring &httpProxy,
                                               unsigned int httpPort, const std::wstring &socksProxy, unsigned int socksPort,
                                               HANDLE clientPipe, unsigned int &outPort, std::string &outDetail)
{
    outPort = 0;
    outDetail.clear();

    msgChannel_.stop();
    // Release any handle from a previous run before launching a new process.
    openVpnProcess_.closeHandle();

    wsl::Win32Handle userToken;
    if (!Utils::getPipeClientPrimaryToken(clientPipe, userToken.data())) {
        spdlog::error("runOpenvpn: could not obtain the pipe client's primary token");
        return ExecuteCmdResult();
    }

    // Same folder as client app log. OpenVPN creates and truncates the file itself; the user profile
    // ACL already lets that unelevated process write it and keeps other users out.
    const std::wstring logFolder = Utils::getClientLogFolder(userToken.getHandle());
    if (logFolder.empty()) {
        spdlog::error("runOpenvpn: client log folder is unavailable for OpenVPN logging");
        return ExecuteCmdResult();
    }
    logPath_ = logFolder + L"\\openvpn.log";

    std::wstring filename;
    std::wstring extraArgs;
    if (!writeOVPNFile(config, httpProxy, httpPort, socksProxy, socksPort, userToken.getHandle(), filename, extraArgs)) {
        return ExecuteCmdResult();
    }

    const std::wstring ovpnExe = Utils::getExePath() + L"\\" WS_PRODUCT_NAME_LOWER_W L"openvpn.exe";

#if defined(USE_SIGNATURE_CHECK)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(ovpnExe)) {
        spdlog::error("OpenVPN service signature incorrect: {}", sigCheck.lastError());
        return ExecuteCmdResult();
    }
#endif

    if (!msgChannel_.create()) {
        spdlog::error("runOpenvpn: could not create msg-channel pipe");
        return ExecuteCmdResult();
    }

    const std::wstring cwd = Utils::getDirPathFromFullPath(filename);
    const HANDLE msgClient = msgChannel_.clientHandle();
    // --log before --config. OpenVPN fully parses the file when it sees --config, and a
    // non-whitelisted directive is fatal. --log is what creates the file, so it has to be parsed
    // first or that fatal exits before the file exists. A log directive in the file cannot
    // replace it: the parser allowlist rejects that name, and the redirect ignores a second
    // --log. Later arguments still override the file. --msg-channel stays last: it is a handle,
    // not a config directive.
    const std::wstring strCmd = L"\"" + ovpnExe + L"\" --log \"" + logPath_ + L"\" --config \""
                                + filename + L"\"" + extraArgs + L" --msg-channel "
                                + std::to_wstring(static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(msgClient)));

    HANDLE processHandle = NULL;
    ExecuteCmdResult res = ExecuteCmd::instance().executeNonblockingCmdAsUser(
        ovpnExe, strCmd, cwd, userToken.getHandle(), msgClient, &processHandle);
    msgChannel_.closeClientHandle();
    if (!res.success) {
        msgChannel_.stop();
        return res;
    }
    openVpnProcess_.setHandle(processHandle);

    if (!msgChannel_.start(openVpnProcess_.getHandle())) {
        spdlog::error("runOpenvpn: could not start msg-channel worker");
        if (openVpnProcess_.isValid()) {
            ::TerminateProcess(openVpnProcess_.getHandle(), 1);
            openVpnProcess_.closeHandle();
        }
        msgChannel_.stop();
        res.success = false;
        return res;
    }

    // OpenVPN binds its management socket on an OS-assigned port (the config requests
    // "management 127.0.0.1 0"). Resolve that port from the spawned process so the engine connects
    // to the exact, unpredictable port the genuine OpenVPN owns.
    if (!resolveManagementPort(res.processId, outPort, outDetail)) {
        spdlog::error("Could not resolve OpenVPN management port for pid {}", res.processId);
        // Kill the process we just launched; without a resolvable management port it is unusable.
        // resolveManagementPort already read the early-exit log, so the kill cannot race the read.
        terminateProcess();
        res.success = false;
        return res;
    }

    spdlog::info("OpenVPN running unelevated, pid {}, management port {}", res.processId, outPort);
    return res;
}

bool OpenVPNController::resolveManagementPort(unsigned long pid, unsigned int &outPort, std::string &outDetail)
{
    outPort = 0;
    outDetail.clear();

    // OpenVPN starts in "management-hold", so it binds the management listener early. Poll for the
    // single loopback (127.0.0.1) TCP listener owned by the OpenVPN process. OpenVPN connects out
    // to its server, so the management socket is the only socket it listens on.
    // The window is generous (~10s): the socket itself binds in milliseconds, but on a slow/busy
    // machine — or with AV/EDR scanning the binary on first launch — the process can take seconds
    // just to start. The engine blocks on this IPC reply with no timeout, so a longer wait is safe.
    const DWORD kLoopbackAddr = htonl(INADDR_LOOPBACK);
    // 200 x 50ms. Shared with the timeout log so the reported attempt count cannot drift.
    constexpr int kResolveAttempts = 200;
    constexpr std::size_t kMaxLoggedRows = 8;

    DWORD lastStatus = ERROR_INVALID_FUNCTION;
    DWORD lastEntryCount = 0;
    DWORD ownedCount = 0;
    int successfulReads = 0;
    int failedReads = 0;
    struct OwnedRow {
        DWORD state;
        DWORD localAddr;
        DWORD localPort;
        DWORD remoteAddr;
        DWORD remotePort;
    };
    std::vector<OwnedRow> ownedRows;

    DWORD dwSize = sizeof(MIB_TCPTABLE2);
    std::unique_ptr<BYTE[]> buffer(new BYTE[dwSize]);
    for (int attempt = 0; attempt < kResolveAttempts; ++attempt) {
        DWORD dwRetVal = ERROR_INSUFFICIENT_BUFFER;
        for (int bufferAttempt = 0; bufferAttempt < 5; ++bufferAttempt) {
            dwRetVal = GetTcpTable2(reinterpret_cast<PMIB_TCPTABLE2>(buffer.get()), &dwSize, TRUE);
            if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) {
                break;
            }
            buffer.reset(new BYTE[dwSize]);
        }
        lastStatus = dwRetVal;

        if (dwRetVal == NO_ERROR) {
            ++successfulReads;
            const auto *pTcpTable = reinterpret_cast<const MIB_TCPTABLE2 *>(buffer.get());
            lastEntryCount = pTcpTable->dwNumEntries;
            // Keep the last successful snapshot. A later failed read must not erase it.
            ownedRows.clear();
            ownedCount = 0;
            for (DWORD i = 0; i < pTcpTable->dwNumEntries; ++i) {
                const auto *entry = &pTcpTable->table[i];
                if (entry->dwOwningPid != pid) {
                    continue;
                }
                ++ownedCount;
                if (ownedRows.size() < kMaxLoggedRows) {
                    ownedRows.push_back(OwnedRow{entry->dwState, entry->dwLocalAddr, entry->dwLocalPort,
                                                 entry->dwRemoteAddr, entry->dwRemotePort});
                }
                if (isManagementListener(entry, kLoopbackAddr)) {
                    outPort = ntohs(static_cast<u_short>(entry->dwLocalPort));
                    return true;
                }
            }
        } else {
            ++failedReads;
        }

        // Bail out early if OpenVPN has already exited (bad config, AV/EDR kill, immediate fatal).
        // A dead process will never bind the management socket, so there is no point polling for the
        // full window and stalling the helper's single command thread. Test liveness via the retained
        // process handle rather than the PID, to avoid a false negative from PID reuse.
        if (openVpnProcess_.isSignaled()) {
            spdlog::error("OpenVPN process (pid {}) exited before binding its management port", pid);
            outDetail = OvpnEarlyExitLog::readFile(logPath_);
            if (!outDetail.empty()) {
                spdlog::error("OpenVPN early-exit log: {}", outDetail);
            }
            return false;
        }

        Sleep(50);
    }

    // The caller only records that resolution failed. Without this, a live daemon whose listener
    // never matches (API error, empty table, or a row the filter rejects) leaves no trace here.
    spdlog::error(
        "OpenVPN management port not found for pid {} after {} attempts (GetTcpTable2 last status {}, successful reads {}, failed reads {}, last table entries {}, owned rows {})",
        pid, kResolveAttempts, lastStatus, successfulReads, failedReads, lastEntryCount, ownedCount);
    if (successfulReads == 0) {
        spdlog::error("GetTcpTable2 never succeeded while resolving the management port for pid {}", pid);
    } else if (ownedRows.empty()) {
        spdlog::error("Last TCP snapshot contained no rows owned by pid {}", pid);
    } else {
        for (const auto &row : ownedRows) {
            const DWORD local = ntohl(row.localAddr);
            const DWORD remote = ntohl(row.remoteAddr);
            spdlog::error(
                "OpenVPN pid {} TCP row state {} ({}) local {}.{}.{}.{}:{} remote {}.{}.{}.{}:{} raw-remote-port {}",
                pid, row.state, tcpStateName(row.state),
                (local >> 24) & 0xffu, (local >> 16) & 0xffu, (local >> 8) & 0xffu, local & 0xffu,
                ntohs(static_cast<u_short>(row.localPort)),
                (remote >> 24) & 0xffu, (remote >> 16) & 0xffu, (remote >> 8) & 0xffu, remote & 0xffu,
                ntohs(static_cast<u_short>(row.remotePort)), row.remotePort);
        }
        if (ownedCount > ownedRows.size()) {
            spdlog::error("OpenVPN pid {} has {} IPv4 TCP rows; logged {}", pid, ownedCount, ownedRows.size());
        }
    }

    return false;
}

bool OpenVPNController::writeOVPNFile(std::wstring &config, const std::wstring &httpProxy, unsigned int httpPort,
                                      const std::wstring &socksProxy, unsigned int socksPort, HANDLE userToken,
                                      std::wstring &filename, std::wstring &extraArgs)
{
    // Replace the deprecated (as of OpenVPN 2.5) ciphers flag received from the server API with the proper one.
    boost::replace_all(config, L"ncp-ciphers", L"data-ciphers");

    if (useDCODriver_) {
        // DCO driver on Windows will not accept the AES-256-CBC cipher and will drop back to using wintun if it is provided in the ciphers list.
        // TODO: implement a smarter algorithm for replacement here in case the list of ciphers received from the server API ever changes.
        boost::ireplace_all(config, L":AES-256-CBC:", L":");
    }

    std::string narrowConfig;
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        narrowConfig = converter.to_bytes(config);
    } catch (const std::range_error &) {
        spdlog::error("OpenVPN config is not valid UTF-16");
        return false;
    }

    std::wstring filePath = Utils::getConfigPath();
    if (filePath.empty()) {
        spdlog::error("Could not get config path");
        return false;
    }

    filePath += L"\\config.ovpn";

    // Remove any prior config before filtering so a rejected config never leaves a stale file behind.
    DeleteFileW(filePath.c_str());

    std::string filtered;
    if (!OvpnDirectiveWhitelist::filterConfig(narrowConfig, filtered)) {
        return false;
    }

    spdlog::debug("Writing OpenVPN config");

    // The filtered config can carry custom-config inline keys and credentials. Grant the pipe
    // client's user SID read so the unelevated OpenVPN process can open the file; SY/BA keep FA.
    DWORD sidLen = 0;
    ::GetTokenInformation(userToken, TokenUser, nullptr, 0, &sidLen);
    if (sidLen == 0) {
        spdlog::error("writeOVPNFile: GetTokenInformation(size) failed: {}", ::GetLastError());
        return false;
    }
    std::vector<BYTE> sidBuf(sidLen);
    if (!::GetTokenInformation(userToken, TokenUser, sidBuf.data(), sidLen, &sidLen)) {
        spdlog::error("writeOVPNFile: GetTokenInformation failed: {}", ::GetLastError());
        return false;
    }
    const PSID userSid = reinterpret_cast<TOKEN_USER *>(sidBuf.data())->User.Sid;
    if (!Utils::createRestrictedFileForUser(filePath, userSid)) {
        return false;
    }

    std::wofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file) {
        spdlog::error("Could not open config file: {}", GetLastError());
        return false;
    }

    // Write filtered config (filterConfig outputs \n; OpenVPN handles both \n and \r\n).
    // Helper-owned directives are not written here. They go on the command line, which the
    // parser allowlist trusts; the same names in this file are rejected.
    file << filtered.c_str();
    
    file.flush();
    if (file.fail()) {
        spdlog::error("OpenVPNController::writeOVPNFile - failed to write config file");
        return false;
    }
    file.close();

    filename = filePath;

    // Port 0 tells OpenVPN to bind an OS-assigned (ephemeral) management port. The helper resolves
    // the actual port from the spawned process and reports it to the engine. This removes the
    // engine-side port pre-selection that left a window for a local process to grab the port first.
    // --dev-node restricts OpenVPN to the DCO/wintun adapter instance we create, so it cannot bind
    // an adapter created by other software (e.g. the vanilla OpenVPN client app).
    extraArgs = L" --management 127.0.0.1 0 --management-query-passwords --management-hold --verb 3"
                L" --dev tun --dev-node " + std::wstring(kOpenVPNAdapterIdentifier);

    // OpenVPN 2.7 removed --windows-driver; the driver is now auto-selected from DCO availability.
    // For the non-DCO path we must explicitly disable DCO, otherwise OpenVPN picks DCO whenever the
    // ovpn-dco-win driver is loaded on the system (e.g. left over from a prior DCO connection) and
    // then bails with an adapter-driver-mismatch fatal against our tap-windows6 adapter.
    if (!useDCODriver_) {
        extraArgs += L" --disable-dco";

        // OpenVPN requires the wintun driver when a proxy is set. The app requests the matching
        // adapter before it asks us to start the daemon.
        if (httpProxy.length() > 0) {
            extraArgs += L" --http-proxy " + httpProxy + L" " + std::to_wstring(httpPort) + L" auto";
        } else if (socksProxy.length() > 0) {
            extraArgs += L" --socks-proxy " + socksProxy + L" " + std::to_wstring(socksPort);
        }
    }

    if (config.find(L"remote 127.0.0.1") != std::wstring::npos) {
        extraArgs += L" --pull-filter ignore redirect-gateway"
                     L" --route 0.0.0.0 128.0.0.0 vpn_gateway"
                     L" --route 128.0.0.0 128.0.0.0 vpn_gateway";
    }

    return true;
}
