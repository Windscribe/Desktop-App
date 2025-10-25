target_sources(com.windscribe.client.networkextension PRIVATE
    FlowTCP.mm
    FlowUDP.mm
    ip_hostnames/ares_library_init.cpp
    ip_hostnames/dns_resolver.cpp
    ip_hostnames/ip_hostnames_manager.cpp
    Settings.mm
    TransparentProxyProvider.mm
    Utils.mm
)
