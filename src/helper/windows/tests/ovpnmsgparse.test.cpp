// Parse tests for the OpenVPN 2.7.5 msg-channel decoder. These pin that a truncated, oversized,
// or unknown payload is rejected before the helper applies any privileged change.

#include <WinSock2.h>
#include <ws2tcpip.h>

#include <cstdio>
#include <cstring>

#include "openvpn/ovpn_msg_parse.h"

namespace
{

int g_failures = 0;

bool check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
    return ok;
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

void testTruncated()
{
    VERIFY(parseOvpnMessage(nullptr, 0, nullptr) == OvpnMsgParseResult::Truncated);
    message_header_t hdr{};
    hdr.type = msg_register_dns;
    hdr.size = sizeof(hdr);
    VERIFY(parseOvpnMessage(&hdr, 4, nullptr) == OvpnMsgParseResult::Truncated);
}

void testSizeMismatch()
{
    address_message_t msg{};
    msg.header.type = msg_add_address;
    msg.header.size = sizeof(msg) - 1;
    msg.family = AF_INET;
    msg.prefix_len = 32;
    VERIFY(parseOvpnMessage(&msg, sizeof(msg), nullptr) == OvpnMsgParseResult::SizeMismatch);

    msg.header.size = sizeof(msg);
    VERIFY(parseOvpnMessage(&msg, sizeof(msg) - 1, nullptr) == OvpnMsgParseResult::SizeMismatch);
}

void testUnknownType()
{
    message_header_t hdr{};
    hdr.type = static_cast<message_type_t>(250);
    hdr.size = sizeof(hdr);
    VERIFY(parseOvpnMessage(&hdr, sizeof(hdr), nullptr) == OvpnMsgParseResult::UnknownType);

    hdr.type = deprecated_msg_register_ring_buffers;
    VERIFY(parseOvpnMessage(&hdr, sizeof(hdr), nullptr) == OvpnMsgParseResult::UnknownType);
}

void testInvalidField()
{
    address_message_t addr{};
    addr.header.type = msg_add_address;
    addr.header.size = sizeof(addr);
    addr.family = AF_UNSPEC;
    addr.prefix_len = 32;
    VERIFY(parseOvpnMessage(&addr, sizeof(addr), nullptr) == OvpnMsgParseResult::InvalidField);

    addr.family = AF_INET;
    addr.prefix_len = 33;
    VERIFY(parseOvpnMessage(&addr, sizeof(addr), nullptr) == OvpnMsgParseResult::InvalidField);

    set_mtu_message_t mtu{};
    mtu.header.type = msg_set_mtu;
    mtu.header.size = sizeof(mtu);
    mtu.family = AF_INET;
    mtu.mtu = 10;
    VERIFY(parseOvpnMessage(&mtu, sizeof(mtu), nullptr) == OvpnMsgParseResult::InvalidField);

    nrpt_dns_cfg_message_t nrpt{};
    nrpt.header.type = msg_add_nrpt_cfg;
    nrpt.header.size = sizeof(nrpt);
    std::memset(nrpt.addresses[0], '1', sizeof(nrpt.addresses[0]));
    VERIFY(parseOvpnMessage(&nrpt, sizeof(nrpt), nullptr) == OvpnMsgParseResult::InvalidField);
    nrpt = {};
    nrpt.header.type = msg_add_nrpt_cfg;
    nrpt.header.size = sizeof(nrpt);
    std::memset(nrpt.resolve_domains, 'x', sizeof(nrpt.resolve_domains));
    nrpt.resolve_domains[sizeof(nrpt.resolve_domains) - 1] = '\0';
    VERIFY(parseOvpnMessage(&nrpt, sizeof(nrpt), nullptr) == OvpnMsgParseResult::InvalidField);

    dns_cfg_message_t dns{};
    dns.header.type = msg_add_dns_cfg;
    dns.header.size = sizeof(dns);
    dns.family = AF_INET;
    std::memset(dns.domains, 'x', sizeof(dns.domains));
    VERIFY(parseOvpnMessage(&dns, sizeof(dns), nullptr) == OvpnMsgParseResult::InvalidField);

    nbt_cfg_message_t nbt{};
    nbt.header.type = msg_add_nbt_cfg;
    nbt.header.size = sizeof(nbt);
    std::memset(nbt.scope_id, 'x', sizeof(nbt.scope_id));
    VERIFY(parseOvpnMessage(&nbt, sizeof(nbt), nullptr) == OvpnMsgParseResult::InvalidField);

    address_message_t iface{};
    iface.header.type = msg_add_address;
    iface.header.size = sizeof(iface);
    iface.family = AF_INET;
    iface.prefix_len = 24;
    std::memset(iface.iface.name, 'x', sizeof(iface.iface.name));
    VERIFY(parseOvpnMessage(&iface, sizeof(iface), nullptr) == OvpnMsgParseResult::InvalidField);
}

void testValidMessages()
{
    message_type_t type = msg_acknowledgement;

    address_message_t addr{};
    addr.header.type = msg_add_address;
    addr.header.size = sizeof(addr);
    addr.family = AF_INET;
    addr.prefix_len = 24;
    VERIFY(parseOvpnMessage(&addr, sizeof(addr), &type) == OvpnMsgParseResult::Ok);
    VERIFY(type == msg_add_address);

    route_message_t route{};
    route.header.type = msg_add_route;
    route.header.size = sizeof(route);
    route.family = AF_INET6;
    route.prefix_len = 64;
    VERIFY(parseOvpnMessage(&route, sizeof(route), &type) == OvpnMsgParseResult::Ok);
    VERIFY(type == msg_add_route);

    message_header_t dns{};
    dns.type = msg_register_dns;
    dns.size = sizeof(dns);
    VERIFY(parseOvpnMessage(&dns, sizeof(dns), &type) == OvpnMsgParseResult::Ok);
    VERIFY(type == msg_register_dns);

    create_adapter_message_t create{};
    create.header.type = msg_create_adapter;
    create.header.size = sizeof(create);
    create.adapter_type = ADAPTER_TYPE_DCO;
    VERIFY(parseOvpnMessage(&create, sizeof(create), &type) == OvpnMsgParseResult::Ok);
    VERIFY(type == msg_create_adapter);

    nrpt_dns_cfg_message_t nrpt{};
    nrpt.header.type = msg_add_nrpt_cfg;
    nrpt.header.size = sizeof(nrpt);
    std::memcpy(nrpt.addresses[0], "1.1.1.1", sizeof("1.1.1.1"));
    VERIFY(parseOvpnMessage(&nrpt, sizeof(nrpt), &type) == OvpnMsgParseResult::Ok);

    set_mtu_message_t mtu{};
    mtu.header.type = msg_set_mtu;
    mtu.header.size = sizeof(mtu);
    mtu.family = AF_INET;
    mtu.mtu = 1500;
    VERIFY(parseOvpnMessage(&mtu, sizeof(mtu), &type) == OvpnMsgParseResult::Ok);
}

} // namespace

int main()
{
    testTruncated();
    testSizeMismatch();
    testUnknownType();
    testInvalidField();
    testValidMessages();

    if (g_failures == 0) {
        printf("ovpnmsgparse: all checks passed\n");
        return 0;
    }
    printf("ovpnmsgparse: %d checks failed\n", g_failures);
    return 1;
}
