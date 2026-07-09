#pragma once

#include <QString>

#include "connectivitydiagnosticresult.h"
#include "types/enums.h"

// Cross-platform helpers shared by the per-platform connectivity-diagnostic
// collectors (section A). Kept here so the win/mac/linux collectors emit
// identical, privacy-safe tokens and detect the proxy mode the same way.
namespace connectivity_diagnostic_collector {

// Maps the platform interface type to a short, privacy-safe token. Never carries
// a network name or SSID, only the adapter media type.
QString interfaceTypeToken(NETWORK_INTERFACE_TYPE type);

// Determines the redacted proxy mode. App-configured proxy takes precedence; if
// none is set we fall back to the system proxy query (the same cross-platform Qt
// path), inspecting only the proxy *type* — never the host, port or credentials.
ProxyMode determineProxyMode();

} // namespace connectivity_diagnostic_collector
