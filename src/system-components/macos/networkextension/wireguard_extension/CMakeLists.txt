target_sources(com.windscribe.client.networkextension PRIVATE
    WireGuardNetworkExtension/ErrorNotifier.swift
    WireGuardNetworkExtension/PacketTunnelProvider.swift
    WireGuardKit/WireGuardAdapter.swift
    WireGuardKit/DnsResolver.swift
    WireGuardKit/IPAddressRange.swift
    WireGuardKit/PacketTunnelSettingsGenerator.swift
    WireGuardKit/PeerConfiguration.swift
    WireGuardKit/PrivateKey.swift
    WireGuardKit/TunnelConfiguration.swift
    WireGuardKit/Endpoint.swift
    WireGuardKit/Array+ConcurrentMap.swift
    WireGuardKit/InterfaceConfiguration.swift
    WireGuardKit/IPAddress+AddrInfo.swift
    WireGuardKit/DNSServer.swift
    WireGuardKitC/x25519.c
    WireGuardKitC/key.c
    Shared/Keychain.swift
    Shared/FileManager+Extension.swift
    Shared/Logging/Logger.swift
    Shared/Logging/ringlogger.cpp
    Shared/Model/TunnelConfiguration+WgQuickConfig.swift
    Shared/Model/NETunnelProviderProtocol+Extension.swift
    Shared/Model/String+ArrayConversion.swift)

# Add libwg-go.a as a static library
add_library(wg-go STATIC IMPORTED)
set_target_properties(wg-go PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/WireGuardKitGo/out/libwg-go.a"
)

# Link against libwg-go.a
target_link_libraries(com.windscribe.client.networkextension PRIVATE wg-go)