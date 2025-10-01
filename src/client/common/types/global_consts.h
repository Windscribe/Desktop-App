#pragma once

#include <string>

static constexpr unsigned long long SIMPLE_CRYPT_KEY = 0x4572A4ACF31A31BA;

static constexpr int MTU_OFFSET_OPENVPN = 40;
static constexpr int MTU_OFFSET_IKEV2 = 80;
static constexpr int MTU_OFFSET_WG = 80;

// The minimum Windows build number we officially support.
static constexpr unsigned int kMinWindowsBuildNumber = 17763;
static constexpr unsigned int kMinWindowsBuildNumberForOpenVPNDCO = 19041;

// The initial production Windows 11 build number.
static constexpr unsigned int kWindows11BuildNumber = 22000;

static const wchar_t* kOpenVPNAdapterIdentifier = L"WindscribeOpenVPN";

static const std::wstring kWireGuardAdapterIdentifier = L"WindscribeWireguard";
static const std::wstring kWireGuardServiceIdentifier = L"WireGuardTunnel$" + kWireGuardAdapterIdentifier;
