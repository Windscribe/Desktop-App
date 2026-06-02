#pragma once

namespace WmiUtils
{
    // True if either the WAN Miniport (IKEv2) or WAN Miniport (IP) pseudo-device reports a
    // non-zero ConfigManagerErrorCode (i.e. is disabled or otherwise unhealthy). Adapter
    // descriptions are hard-coded; nothing from the IPC caller enters the WQL query.
    bool isWanIkev2AdapterDisabled();
}
