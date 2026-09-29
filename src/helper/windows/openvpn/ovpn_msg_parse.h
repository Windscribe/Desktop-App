#pragma once

#include <Windows.h>

#include "openvpn-msg.h"

enum class OvpnMsgParseResult {
    Ok,
    Truncated,
    SizeMismatch,
    UnknownType,
    InvalidField
};

OvpnMsgParseResult parseOvpnMessage(const void *data, DWORD size, message_type_t *outType);
