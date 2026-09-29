#include <winsock2.h>
#include <ws2tcpip.h>

#include "ovpn_msg_parse.h"

#include <cstring>

namespace {

bool familyOk(short family)
{
    return family == AF_INET || family == AF_INET6;
}

bool prefixLenOk(short family, int prefixLen)
{
    if (prefixLen < 0) {
        return false;
    }
    if (family == AF_INET) {
        return prefixLen <= 32;
    }
    return prefixLen <= 128;
}

template <size_t N>
bool isNullTerminated(const char (&value)[N])
{
    return std::memchr(value, '\0', N) != nullptr;
}

template <size_t N>
bool isDoubleNullTerminated(const char (&value)[N])
{
    for (size_t i = 1; i < N; ++i) {
        if (value[i - 1] == '\0' && value[i] == '\0') {
            return true;
        }
    }
    return false;
}

bool interfaceOk(const interface_t &iface)
{
    return isNullTerminated(iface.name);
}

} // namespace

OvpnMsgParseResult parseOvpnMessage(const void *data, DWORD size, message_type_t *outType)
{
    if (data == nullptr || size < sizeof(message_header_t)) {
        return OvpnMsgParseResult::Truncated;
    }

    const auto *hdr = static_cast<const message_header_t *>(data);
    if (hdr->size != size) {
        return OvpnMsgParseResult::SizeMismatch;
    }
    if (outType != nullptr) {
        *outType = hdr->type;
    }

    switch (hdr->type) {
    case msg_add_address:
    case msg_del_address: {
        if (size != sizeof(address_message_t)) {
            return OvpnMsgParseResult::SizeMismatch;
        }
        const auto *m = static_cast<const address_message_t *>(data);
        if (!familyOk(m->family) || !prefixLenOk(m->family, m->prefix_len) || !interfaceOk(m->iface)) {
            return OvpnMsgParseResult::InvalidField;
        }
        return OvpnMsgParseResult::Ok;
    }
    case msg_add_route:
    case msg_del_route: {
        if (size != sizeof(route_message_t)) {
            return OvpnMsgParseResult::SizeMismatch;
        }
        const auto *m = static_cast<const route_message_t *>(data);
        if (!familyOk(m->family) || !prefixLenOk(m->family, m->prefix_len) || !interfaceOk(m->iface)) {
            return OvpnMsgParseResult::InvalidField;
        }
        return OvpnMsgParseResult::Ok;
    }
    case msg_flush_neighbors:
        if (size != sizeof(flush_neighbors_message_t)) {
            return OvpnMsgParseResult::SizeMismatch;
        }
        {
            const auto *m = static_cast<const flush_neighbors_message_t *>(data);
            if (!familyOk(m->family) || !interfaceOk(m->iface)) {
                return OvpnMsgParseResult::InvalidField;
            }
        }
        return OvpnMsgParseResult::Ok;
    case msg_add_dns_cfg:
    case msg_del_dns_cfg: {
        if (size != sizeof(dns_cfg_message_t)) {
            return OvpnMsgParseResult::SizeMismatch;
        }
        const auto *m = static_cast<const dns_cfg_message_t *>(data);
        if (!familyOk(m->family) || m->addr_len < 0 || m->addr_len > 4
            || !interfaceOk(m->iface) || !isNullTerminated(m->domains)) {
            return OvpnMsgParseResult::InvalidField;
        }
        return OvpnMsgParseResult::Ok;
    }
    case msg_add_nrpt_cfg:
    case msg_del_nrpt_cfg: {
        if (size != sizeof(nrpt_dns_cfg_message_t)) return OvpnMsgParseResult::SizeMismatch;
        const auto *m = static_cast<const nrpt_dns_cfg_message_t *>(data);
        if (!interfaceOk(m->iface) || !isDoubleNullTerminated(m->resolve_domains) || !isNullTerminated(m->search_domains)) return OvpnMsgParseResult::InvalidField;
        for (const auto &address : m->addresses) if (!isNullTerminated(address)) return OvpnMsgParseResult::InvalidField;
        return OvpnMsgParseResult::Ok;
    }
    case msg_add_wins_cfg:
    case msg_del_wins_cfg: {
        if (size != sizeof(wins_cfg_message_t)) return OvpnMsgParseResult::SizeMismatch;
        const auto *m = static_cast<const wins_cfg_message_t *>(data);
        return interfaceOk(m->iface) && m->addr_len <= 4 ? OvpnMsgParseResult::Ok : OvpnMsgParseResult::InvalidField;
    }
    case msg_add_nbt_cfg:
    case msg_del_nbt_cfg: {
        if (size != sizeof(nbt_cfg_message_t)) return OvpnMsgParseResult::SizeMismatch;
        const auto *m = static_cast<const nbt_cfg_message_t *>(data);
        return interfaceOk(m->iface) && isNullTerminated(m->scope_id) ? OvpnMsgParseResult::Ok : OvpnMsgParseResult::InvalidField;
    }
    case msg_add_wfp_block:
    case msg_del_wfp_block:
        if (size != sizeof(wfp_block_message_t)) return OvpnMsgParseResult::SizeMismatch;
        return interfaceOk(static_cast<const wfp_block_message_t *>(data)->iface) ? OvpnMsgParseResult::Ok : OvpnMsgParseResult::InvalidField;
    case msg_enable_dhcp:
        if (size != sizeof(enable_dhcp_message_t)) return OvpnMsgParseResult::SizeMismatch;
        return interfaceOk(static_cast<const enable_dhcp_message_t *>(data)->iface) ? OvpnMsgParseResult::Ok : OvpnMsgParseResult::InvalidField;
    case msg_set_mtu: {
        if (size != sizeof(set_mtu_message_t)) {
            return OvpnMsgParseResult::SizeMismatch;
        }
        const auto *m = static_cast<const set_mtu_message_t *>(data);
        if (!familyOk(m->family) || m->mtu < 68 || m->mtu > 9000 || !interfaceOk(m->iface)) {
            return OvpnMsgParseResult::InvalidField;
        }
        return OvpnMsgParseResult::Ok;
    }
    case msg_create_adapter:
        if (size != sizeof(create_adapter_message_t)) {
            return OvpnMsgParseResult::SizeMismatch;
        }
        {
            const auto type = static_cast<const create_adapter_message_t *>(data)->adapter_type;
            if (type != ADAPTER_TYPE_DCO && type != ADAPTER_TYPE_TAP) {
                return OvpnMsgParseResult::InvalidField;
            }
        }
        return OvpnMsgParseResult::Ok;
    case msg_register_dns:
        return OvpnMsgParseResult::Ok;
    case msg_acknowledgement:
    case deprecated_msg_register_ring_buffers:
        return OvpnMsgParseResult::UnknownType;
    default:
        return OvpnMsgParseResult::UnknownType;
    }
}
