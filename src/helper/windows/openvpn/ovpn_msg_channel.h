#pragma once

#include <Windows.h>

#include "openvpn-msg.h"
#include "utils/win32handle.h"

// Local union over the OpenVPN 2.7 msg-channel structs (not in upstream openvpn-msg.h).
typedef union {
    message_header_t header;
    address_message_t address;
    route_message_t route;
    dns_cfg_message_t dns;
    nrpt_dns_cfg_message_t nrpt_dns;
    nbt_cfg_message_t nbt;
    flush_neighbors_message_t flush_neighbors;
    ack_message_t ack;
    wfp_block_message_t wfp_block;
    enable_dhcp_message_t dhcp;
    set_mtu_message_t mtu;
    wins_cfg_message_t wins;
    create_adapter_message_t create_adapter;
} pipe_message_t;

// Duplex named-pipe worker that speaks OpenVPN 2.7.5 --msg-channel. The helper creates the pipe,
// the child inherits only the client end, and this object services privileged requests on a
// dedicated thread so the helper IPC thread stays free.
class OvpnMsgChannel
{
public:
    OvpnMsgChannel() = default;
    ~OvpnMsgChannel();

    OvpnMsgChannel(const OvpnMsgChannel &) = delete;
    OvpnMsgChannel &operator=(const OvpnMsgChannel &) = delete;

    // Creates the duplex pipe. On success clientHandle() is inheritable and must be passed to
    // CreateProcessAsUser, then closeClientHandle() called in the parent.
    bool create();
    HANDLE clientHandle() const { return clientPipe_.getHandle(); }
    void closeClientHandle();

    // Duplicates processHandle and starts the worker. processHandle remains owned by the caller.
    bool start(HANDLE processHandle);
    void stop();

    // Apply a parsed message (also used by tests with crafted payloads). Returns the Win32 error
    // placed in ack.error_number (NO_ERROR on success).
    static DWORD handleMessage(const pipe_message_t &msg);

private:
    wsl::Win32Handle serverPipe_;
    wsl::Win32Handle clientPipe_;
    wsl::Win32Handle stopEvent_;
    wsl::Win32Handle thread_;
    wsl::Win32Handle process_;

    static DWORD WINAPI workerThread(LPVOID param);
    void workerLoop();
    void undoPrivilegedState();
    bool writeAck(int messageId, DWORD errorNumber, HANDLE writeEvent);

    static DWORD handleAddress(const address_message_t &msg);
    static DWORD handleRoute(const route_message_t &msg);
    static DWORD handleFlushNeighbors(const flush_neighbors_message_t &msg);
    static DWORD handleDnsCfg(const dns_cfg_message_t &msg);
    static DWORD handleNrptCfg(const nrpt_dns_cfg_message_t &msg);
    static DWORD handleMtu(const set_mtu_message_t &msg);
    static DWORD handleRegisterDns();
};
