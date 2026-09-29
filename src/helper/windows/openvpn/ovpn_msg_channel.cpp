#include "ws_branding.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <sddl.h>

#include "ovpn_msg_channel.h"
#include "ovpn_msg_parse.h"

#include <cstring>
#include <vector>
#include <spdlog/spdlog.h>

#include "../executecmd.h"
#include "../utils.h"
#include "utils/crashhandler.h"
#include "utils/systemlibloader.h"
#include "utils/wsscopeguard.h"

namespace {

constexpr DWORD kErrorMessageData = 0x20000002;
constexpr DWORD kErrorMessageType = 0x20000003;

bool hasEmbeddedNul(const char *s, size_t n)
{
    return s != nullptr && std::memchr(s, 0, n) != nullptr;
}

SOCKADDR_INET sockaddrFromInet(short family, const inet_address_t &addr)
{
    SOCKADDR_INET sa{};
    if (family == AF_INET) {
        sa.Ipv4.sin_family = AF_INET;
        sa.Ipv4.sin_addr = addr.ipv4;
    } else {
        sa.Ipv6.sin6_family = AF_INET6;
        sa.Ipv6.sin6_addr = addr.ipv6;
    }
    return sa;
}

DWORD resolveIface(const interface_t &iface, NET_IFINDEX &index, NET_LUID &luid)
{
    if (iface.index != -1) {
        index = static_cast<NET_IFINDEX>(iface.index);
        const DWORD err = ::ConvertInterfaceIndexToLuid(index, &luid);
        return err;
    }
    if (!hasEmbeddedNul(iface.name, sizeof(iface.name))) {
        return kErrorMessageData;
    }
    wchar_t alias[256] = {};
    if (::MultiByteToWideChar(CP_UTF8, 0, iface.name, -1, alias, _countof(alias)) <= 0) {
        return ::GetLastError();
    }
    const DWORD err = ::ConvertInterfaceAliasToLuid(alias, &luid);
    if (err != NO_ERROR) {
        return err;
    }
    return ::ConvertInterfaceLuidToIndex(&luid, &index);
}

bool appendNameServer(std::vector<std::wstring> &servers, short family, const void *addr)
{
    wchar_t buf[64] = {};
    if (::InetNtopW(family, addr, buf, _countof(buf)) == nullptr || buf[0] == L'\0') {
        return false;
    }
    servers.push_back(buf);
    return true;
}

DWORD setInterfaceDns(NET_IFINDEX ifIndex, short family, const std::vector<std::wstring> &nameServers)
{
    try {
        wsl::SystemLibLoader iphlpapi("iphlpapi.dll");
        const auto pSetInterfaceDnsSettings =
            iphlpapi.getFunction<DWORD WINAPI(GUID, const DNS_INTERFACE_SETTINGS *)>("SetInterfaceDnsSettings");

        NET_LUID luid = {};
        DWORD ret = ::ConvertInterfaceIndexToLuid(ifIndex, &luid);
        if (ret != NO_ERROR) {
            return ret;
        }
        GUID guid = {};
        ret = ::ConvertInterfaceLuidToGuid(&luid, &guid);
        if (ret != NO_ERROR) {
            return ret;
        }

        std::wstring nameServer;
        for (const auto &s : nameServers) {
            if (!nameServer.empty()) {
                nameServer += L',';
            }
            nameServer += s;
        }
        DNS_INTERFACE_SETTINGS settings = {};
        settings.Version = DNS_INTERFACE_SETTINGS_VERSION1;
        settings.Flags = DNS_SETTING_NAMESERVER | (family == AF_INET6 ? DNS_SETTING_IPV6 : 0);
        settings.NameServer = nameServer.empty() ? nullptr : nameServer.data();
        return pSetInterfaceDnsSettings(guid, &settings);
    }
    catch (const std::system_error &ex) {
        // SetInterfaceDnsSettings unavailable on this OS — fall through to the netsh path below.
        spdlog::debug("OvpnMsgChannel setInterfaceDns: SetInterfaceDnsSettings API unavailable ({}), falling back to netsh", ex.what());
    }

    auto runNetsh = [](const std::wstring &cmd) -> DWORD {
        const auto res = ExecuteCmd::instance().executeBlockingCmd(cmd);
        return (res.success && res.exitCode == 0) ? NO_ERROR : (res.exitCode ? res.exitCode : ERROR_GEN_FAILURE);
    };

    std::wstring prefix = Utils::getSystemDir() + L"\\netsh.exe interface ";
    prefix += (family == AF_INET6) ? L"ipv6" : L"ipv4";
    const std::wstring nameArg = L" name=" + std::to_wstring(ifIndex);
    if (nameServers.empty()) {
        return runNetsh(prefix + L" set dns" + nameArg + L" dhcp");
    }

    // netsh set dns ... static takes one address; additional servers use add dns.
    DWORD err = runNetsh(prefix + L" set dns" + nameArg + L" static " + nameServers.front());
    if (err != NO_ERROR) {
        return err;
    }
    for (size_t i = 1; i < nameServers.size(); ++i) {
        err = runNetsh(prefix + L" add dns" + nameArg + L" " + nameServers[i]);
        if (err != NO_ERROR) {
            return err;
        }
    }
    return NO_ERROR;
}

} // namespace

OvpnMsgChannel::~OvpnMsgChannel()
{
    stop();
}

bool OvpnMsgChannel::create()
{
    stop();

    static volatile LONG serial = 0;
    const LONG n = ::InterlockedIncrement(&serial);

    wchar_t pipeName[256];
    if (swprintf_s(pipeName, _countof(pipeName), L"\\\\.\\pipe\\" WS_APP_IDENTIFIER_W L"ServiceOvpnMsg.%lu.%ld",
                   ::GetCurrentProcessId(), n) < 0) {
        return false;
    }

    SECURITY_ATTRIBUTES saNamed{};
    saNamed.nLength = sizeof(saNamed);
    saNamed.bInheritHandle = FALSE;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;;GA;;;SY)(A;;GA;;;BA)", SDDL_REVISION_1, &saNamed.lpSecurityDescriptor, nullptr)) {
        spdlog::error("OvpnMsgChannel CreateNamedPipe SD failed: {}", ::GetLastError());
        return false;
    }
    auto freeSd = wsl::wsScopeGuard([&] {
        ::LocalFree(saNamed.lpSecurityDescriptor);
    });

    serverPipe_.setHandle(::CreateNamedPipeW(
        pipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, 4096, 4096, 0, &saNamed));
    if (!serverPipe_.isValid()) {
        spdlog::error("OvpnMsgChannel CreateNamedPipe failed: {}", ::GetLastError());
        return false;
    }

    SECURITY_ATTRIBUTES saInherit{};
    saInherit.nLength = sizeof(saInherit);
    saInherit.bInheritHandle = TRUE;

    clientPipe_.setHandle(::CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, &saInherit,
                                        OPEN_EXISTING, 0, nullptr));
    if (!clientPipe_.isValid()) {
        spdlog::error("OvpnMsgChannel CreateFile(client) failed: {}", ::GetLastError());
        serverPipe_.closeHandle();
        return false;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!::SetNamedPipeHandleState(clientPipe_.getHandle(), &mode, nullptr, nullptr)) {
        spdlog::error("OvpnMsgChannel SetNamedPipeHandleState failed: {}", ::GetLastError());
        clientPipe_.closeHandle();
        serverPipe_.closeHandle();
        return false;
    }

    return true;
}

void OvpnMsgChannel::closeClientHandle()
{
    clientPipe_.closeHandle();
}

bool OvpnMsgChannel::start(HANDLE processHandle)
{
    if (!serverPipe_.isValid() || processHandle == nullptr || processHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    HANDLE dup = nullptr;
    if (!::DuplicateHandle(::GetCurrentProcess(), processHandle, ::GetCurrentProcess(), &dup,
                           SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0)) {
        spdlog::error("OvpnMsgChannel DuplicateHandle(process) failed: {}", ::GetLastError());
        return false;
    }
    process_.setHandle(dup);

    stopEvent_.setHandle(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!stopEvent_.isValid()) {
        spdlog::error("OvpnMsgChannel CreateEvent failed: {}", ::GetLastError());
        process_.closeHandle();
        return false;
    }

    thread_.setHandle(::CreateThread(nullptr, 0, workerThread, this, 0, nullptr));
    if (!thread_.isValid()) {
        spdlog::error("OvpnMsgChannel CreateThread failed: {}", ::GetLastError());
        stopEvent_.closeHandle();
        process_.closeHandle();
        return false;
    }
    return true;
}

void OvpnMsgChannel::stop()
{
    if (stopEvent_.isValid()) {
        ::SetEvent(stopEvent_.getHandle());
    }
    if (serverPipe_.isValid()) {
        ::CancelIoEx(serverPipe_.getHandle(), nullptr);
    }
    if (thread_.isValid()) {
        thread_.wait(INFINITE);
        thread_.closeHandle();
    }
    stopEvent_.closeHandle();
    process_.closeHandle();
    clientPipe_.closeHandle();
    serverPipe_.closeHandle();
}

DWORD WINAPI OvpnMsgChannel::workerThread(LPVOID param)
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    static_cast<OvpnMsgChannel *>(param)->workerLoop();
    return 0;
}

void OvpnMsgChannel::workerLoop()
{
    // OpenVPN's send_msg_iservice blocks in ReadFile for the ack. Disconnecting the server end
    // unblocks it if this thread exits without writing one. stop() joins before closing the handle.
    auto disconnectPipe = wsl::wsScopeGuard([this] {
        if (serverPipe_.isValid()) {
            ::DisconnectNamedPipe(serverPipe_.getHandle());
        }
    });

    wsl::Win32Handle readEvent(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!readEvent.isValid()) {
        spdlog::error("OvpnMsgChannel worker CreateEvent failed: {}", ::GetLastError());
        return;
    }
    wsl::Win32Handle writeEvent(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!writeEvent.isValid()) {
        spdlog::error("OvpnMsgChannel worker CreateEvent failed: {}", ::GetLastError());
        return;
    }

    // Message-mode ReadFile reports ERROR_MORE_DATA when the frame exceeds sizeof(msg).
    auto nackTooLarge = [&](DWORD bytes) {
        spdlog::error("OvpnMsgChannel payload too large ({} bytes)", bytes);
        writeAck(-1, kErrorMessageData, writeEvent.getHandle());
    };

    while (!stopEvent_.isSignaled() && !process_.isSignaled()) {
        pipe_message_t msg{};
        OVERLAPPED ov{};
        ov.hEvent = readEvent.getHandle();
        ::ResetEvent(readEvent.getHandle());

        DWORD bytes = 0;
        BOOL ok = ::ReadFile(serverPipe_.getHandle(), &msg, sizeof(msg), &bytes, &ov);
        if (!ok) {
            const DWORD err = ::GetLastError();
            if (err == ERROR_IO_PENDING) {
                HANDLE waits[3] = { stopEvent_.getHandle(), process_.getHandle(), readEvent.getHandle() };
                const DWORD w = ::WaitForMultipleObjects(3, waits, FALSE, INFINITE);
                if (w != WAIT_OBJECT_0 + 2) {
                    ::CancelIoEx(serverPipe_.getHandle(), &ov);
                    ::GetOverlappedResult(serverPipe_.getHandle(), &ov, &bytes, TRUE);
                    break;
                }
                if (!::GetOverlappedResult(serverPipe_.getHandle(), &ov, &bytes, FALSE)) {
                    if (::GetLastError() == ERROR_MORE_DATA) {
                        nackTooLarge(bytes);
                    }
                    break;
                }
            } else if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA || err == ERROR_OPERATION_ABORTED) {
                break;
            } else if (err == ERROR_MORE_DATA) {
                nackTooLarge(bytes);
                break;
            } else {
                spdlog::error("OvpnMsgChannel ReadFile failed: {}", err);
                break;
            }
        }

        message_type_t type = msg_acknowledgement;
        const OvpnMsgParseResult parsed = parseOvpnMessage(&msg, bytes, &type);
        DWORD ackErr = NO_ERROR;
        if (parsed == OvpnMsgParseResult::UnknownType) {
            ackErr = kErrorMessageType;
        } else if (parsed != OvpnMsgParseResult::Ok) {
            ackErr = kErrorMessageData;
        } else {
            ackErr = handleMessage(msg);
        }
        if (!writeAck(msg.header.message_id, ackErr, writeEvent.getHandle())) {
            break;
        }
    }

    undoPrivilegedState();
}

bool OvpnMsgChannel::writeAck(int messageId, DWORD errorNumber, HANDLE writeEvent)
{
    ack_message_t ack{};
    ack.header.type = msg_acknowledgement;
    ack.header.size = sizeof(ack);
    ack.header.message_id = messageId;
    ack.error_number = static_cast<int>(errorNumber);

    OVERLAPPED ov{};
    ov.hEvent = writeEvent;
    ::ResetEvent(writeEvent);

    DWORD written = 0;
    if (!::WriteFile(serverPipe_.getHandle(), &ack, sizeof(ack), &written, &ov)) {
        const DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
            if (err != ERROR_BROKEN_PIPE && err != ERROR_NO_DATA) {
                spdlog::error("OvpnMsgChannel WriteFile(ack) failed: {}", err);
            }
            return false;
        }

        HANDLE waits[3] = { stopEvent_.getHandle(), process_.getHandle(), writeEvent };
        const DWORD w = ::WaitForMultipleObjects(3, waits, FALSE, INFINITE);
        if (w != WAIT_OBJECT_0 + 2) {
            ::CancelIoEx(serverPipe_.getHandle(), &ov);
            ::GetOverlappedResult(serverPipe_.getHandle(), &ov, &written, TRUE);
            return false;
        }
        if (!::GetOverlappedResult(serverPipe_.getHandle(), &ov, &written, FALSE)) {
            const DWORD ovErr = ::GetLastError();
            if (ovErr != ERROR_BROKEN_PIPE && ovErr != ERROR_NO_DATA && ovErr != ERROR_OPERATION_ABORTED) {
                spdlog::error("OvpnMsgChannel WriteFile(ack) GetOverlappedResult failed: {}", ovErr);
            }
            return false;
        }
    }

    if (written != sizeof(ack)) {
        spdlog::error("OvpnMsgChannel WriteFile(ack) incomplete write: {} {}", written, sizeof(ack));
        return false;
    }

    return true;
}

void OvpnMsgChannel::undoPrivilegedState()
{
    // Addresses and routes are undone by OpenVPN's matching del_* messages on a clean shutdown, and
    // by adapter deletion on the disconnect path. Nothing extra is retained here on purpose: DNS is
    // owned by setCustomDnsWhileConnected / adapter teardown, and a second undo would race with them.
}

DWORD OvpnMsgChannel::handleMessage(const pipe_message_t &msg)
{
    switch (msg.header.type) {
    case msg_add_address:
    case msg_del_address:
        return handleAddress(msg.address);
    case msg_add_route:
    case msg_del_route:
        return handleRoute(msg.route);
    case msg_flush_neighbors:
        return handleFlushNeighbors(msg.flush_neighbors);
    case msg_add_dns_cfg:
    case msg_del_dns_cfg:
        return handleDnsCfg(msg.dns);
    case msg_add_nrpt_cfg:
    case msg_del_nrpt_cfg:
        return handleNrptCfg(msg.nrpt_dns);
    case msg_enable_dhcp:
        // OpenVPN 2.7.5 sends this only for TAP + --ip-win32 dynamic when the adapter has DHCP
        // disabled. DCO rewrites dynamic/adaptive to netsh (msg_add_address). TAP adapters are
        // recreated via tapctl each connect and already have DHCP enabled, so OpenVPN skips this
        // message. Enabling DHCP on a caller-supplied ifindex is unused attack surface.
        spdlog::debug("OvpnMsgChannel: ignoring enable DHCP request");
        return NO_ERROR;
    case msg_set_mtu:
        return handleMtu(msg.mtu);
    case msg_create_adapter:
        // OpenVPN 2.7 only sends this when --dev-node is unset. The helper always writes
        // --dev-node, and adapter lifetime is the IPC createOpenVpnAdapter / removeOpenVpnAdapter
        // path.
        spdlog::debug("OvpnMsgChannel: ignoring create-adapter request (helper owns the named adapter)");
        return NO_ERROR;
    case msg_register_dns:
        return handleRegisterDns();
    case msg_add_wfp_block:
    case msg_del_wfp_block:
        // Windscribe already owns WFP (FirewallFilter / DnsFirewall) and strips block-outside-dns.
        spdlog::debug("OvpnMsgChannel: ignoring WFP-block request (helper firewall owns this)");
        return NO_ERROR;
    case msg_add_wins_cfg:
    case msg_del_wins_cfg:
    case msg_add_nbt_cfg:
    case msg_del_nbt_cfg:
        spdlog::debug("OvpnMsgChannel: ignoring WINS/NBT request");
        return NO_ERROR;
    default:
        return kErrorMessageType;
    }
}

DWORD OvpnMsgChannel::handleAddress(const address_message_t &msg)
{
    NET_IFINDEX index = 0;
    NET_LUID luid{};
    const DWORD err = resolveIface(msg.iface, index, luid);
    if (err != NO_ERROR) {
        spdlog::error("OvpnMsgChannel handleAddress: resolveIface failed ({})", err);
        return err;
    }

    MIB_UNICASTIPADDRESS_ROW row{};
    ::InitializeUnicastIpAddressEntry(&row);
    row.Address = sockaddrFromInet(msg.family, msg.address);
    row.OnLinkPrefixLength = static_cast<UINT8>(msg.prefix_len);
    row.InterfaceIndex = index;
    row.InterfaceLuid = luid;

    if (msg.header.type == msg_add_address) {
        const DWORD addErr = ::CreateUnicastIpAddressEntry(&row);
        if (addErr != NO_ERROR && addErr != ERROR_OBJECT_ALREADY_EXISTS) {
            spdlog::error("OvpnMsgChannel CreateUnicastIpAddressEntry failed ({})", addErr);
            return addErr;
        }
        return NO_ERROR;
    }

    const DWORD delErr = ::DeleteUnicastIpAddressEntry(&row);
    if (delErr != NO_ERROR && delErr != ERROR_NOT_FOUND) {
        spdlog::error("OvpnMsgChannel DeleteUnicastIpAddressEntry failed ({})", delErr);
        return delErr;
    }
    return NO_ERROR;
}

DWORD OvpnMsgChannel::handleRoute(const route_message_t &msg)
{
    NET_IFINDEX index = 0;
    NET_LUID luid{};
    const DWORD err = resolveIface(msg.iface, index, luid);
    if (err != NO_ERROR) {
        spdlog::error("OvpnMsgChannel handleRoute: resolveIface failed ({})", err);
        return err;
    }

    MIB_IPFORWARD_ROW2 row{};
    row.ValidLifetime = 0xffffffff;
    row.PreferredLifetime = 0xffffffff;
    row.Protocol = MIB_IPPROTO_NETMGMT;
    row.Metric = static_cast<ULONG>(msg.metric);
    row.DestinationPrefix.Prefix = sockaddrFromInet(msg.family, msg.prefix);
    row.DestinationPrefix.PrefixLength = static_cast<UINT8>(msg.prefix_len);
    row.NextHop = sockaddrFromInet(msg.family, msg.gateway);
    row.InterfaceIndex = index;
    row.InterfaceLuid = luid;

    if (msg.header.type == msg_add_route) {
        const DWORD addErr = ::CreateIpForwardEntry2(&row);
        if (addErr != NO_ERROR && addErr != ERROR_OBJECT_ALREADY_EXISTS) {
            spdlog::error("OvpnMsgChannel CreateIpForwardEntry2 failed ({})", addErr);
        }
        // OpenVPN maps ERROR_OBJECT_ALREADY_EXISTS to RTA_EEXIST and does not set RT_ADDED,
        // so it will not delete a pre-existing identical route on disconnect.
        return addErr;
    }

    const DWORD delErr = ::DeleteIpForwardEntry2(&row);
    if (delErr != NO_ERROR && delErr != ERROR_NOT_FOUND) {
        spdlog::error("OvpnMsgChannel DeleteIpForwardEntry2 failed ({})", delErr);
        return delErr;
    }
    return NO_ERROR;
}

DWORD OvpnMsgChannel::handleFlushNeighbors(const flush_neighbors_message_t &msg)
{
    NET_IFINDEX index = 0;
    NET_LUID luid{};
    const DWORD err = resolveIface(msg.iface, index, luid);
    if (err != NO_ERROR) {
        return err;
    }
    if (msg.family == AF_INET) {
        return ::FlushIpNetTable(index);
    }
    return ::FlushIpNetTable2(msg.family, index);
}

DWORD OvpnMsgChannel::handleDnsCfg(const dns_cfg_message_t &msg)
{
    NET_IFINDEX index = 0;
    NET_LUID luid{};
    const DWORD err = resolveIface(msg.iface, index, luid);
    if (err != NO_ERROR) {
        return err;
    }

    if (msg.header.type == msg_del_dns_cfg) {
        return setInterfaceDns(index, msg.family, {});
    }

    // Do nothing on an add with no servers supplied.  This early return is hit when the
    // dhcp-option DOMAIN setting is used in a custom config.  Official configs from Windscribe
    // do not use that setting.  In an effort to maximize app security, we have opted to not
    // support all possible custom config settings.
    if (msg.addr_len <= 0) {
        return NO_ERROR;
    }

    std::vector<std::wstring> servers;
    const int n = (msg.addr_len > 4) ? 4 : msg.addr_len;
    for (int i = 0; i < n; ++i) {
        const bool ok = (msg.family == AF_INET6)
            ? appendNameServer(servers, AF_INET6, &msg.addr[i].ipv6)
            : appendNameServer(servers, AF_INET, &msg.addr[i].ipv4);
        if (!ok) {
            return kErrorMessageData;
        }
    }
    return setInterfaceDns(index, msg.family, servers);
}

DWORD OvpnMsgChannel::handleNrptCfg(const nrpt_dns_cfg_message_t &msg)
{
    // Apply the address list as interface DNS rather than writing NRPT policy. Windscribe's
    // connected-DNS path (setCustomDnsWhileConnected) is the authority for resolver config.
    // One NRPT message can mix families; apply each family separately and clear both on delete.
    NET_IFINDEX index = 0;
    NET_LUID luid{};
    const DWORD err = resolveIface(msg.iface, index, luid);
    if (err != NO_ERROR) {
        return err;
    }

    if (msg.header.type == msg_del_nrpt_cfg) {
        const DWORD v4Err = setInterfaceDns(index, AF_INET, {});
        const DWORD v6Err = setInterfaceDns(index, AF_INET6, {});
        return (v4Err != NO_ERROR) ? v4Err : v6Err;
    }

    std::vector<std::wstring> v4Servers;
    std::vector<std::wstring> v6Servers;
    for (int i = 0; i < NRPT_ADDR_NUM; ++i) {
        if (msg.addresses[i][0] == '\0') {
            break;
        }
        IN_ADDR v4{};
        IN6_ADDR v6{};
        if (::InetPtonA(AF_INET, msg.addresses[i], &v4) == 1) {
            if (!appendNameServer(v4Servers, AF_INET, &v4)) {
                return kErrorMessageData;
            }
        } else if (::InetPtonA(AF_INET6, msg.addresses[i], &v6) == 1) {
            if (!appendNameServer(v6Servers, AF_INET6, &v6)) {
                return kErrorMessageData;
            }
        }
    }

    if (!v4Servers.empty()) {
        const DWORD v4Err = setInterfaceDns(index, AF_INET, v4Servers);
        if (v4Err != NO_ERROR) {
            return v4Err;
        }
    }
    if (!v6Servers.empty()) {
        return setInterfaceDns(index, AF_INET6, v6Servers);
    }
    return NO_ERROR;
}

DWORD OvpnMsgChannel::handleMtu(const set_mtu_message_t &msg)
{
    NET_IFINDEX index = 0;
    NET_LUID luid{};
    const DWORD err = resolveIface(msg.iface, index, luid);
    if (err != NO_ERROR) {
        return err;
    }

    MIB_IPINTERFACE_ROW row{};
    ::InitializeIpInterfaceEntry(&row);
    row.Family = static_cast<ADDRESS_FAMILY>(msg.family);
    row.InterfaceIndex = index;
    DWORD getErr = ::GetIpInterfaceEntry(&row);
    if (getErr != NO_ERROR) {
        return getErr;
    }
    if (msg.family == AF_INET) {
        row.SitePrefixLength = 0;
    }
    row.NlMtu = static_cast<ULONG>(msg.mtu);
    return ::SetIpInterfaceEntry(&row);
}

DWORD OvpnMsgChannel::handleRegisterDns()
{
    try {
        wsl::SystemLibLoader dnsapi("dnsapi.dll");
        const auto flush = dnsapi.getFunction<BOOL WINAPI()>("DnsFlushResolverCache");
        return flush() ? NO_ERROR : ::GetLastError();
    }
    catch (const std::system_error &ex) {
        spdlog::warn("OvpnMsgChannel DnsFlushResolverCache unavailable: {}", ex.what());
        return NO_ERROR;
    }
}
