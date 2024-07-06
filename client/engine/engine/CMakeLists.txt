target_sources(engine PRIVATE
    crossplatformobjectfactory.cpp
    crossplatformobjectfactory.h
    engine.cpp
    engine.h
    getdeviceid.cpp
    getdeviceid.h
    logout_helper.h
    openvpnversioncontroller.cpp
    openvpnversioncontroller.h
    packetsizecontroller.cpp
    packetsizecontroller.h
)

if (WIN32)
    target_sources(engine PRIVATE
        adaptermetricscontroller_win.cpp
        adaptermetricscontroller_win.h
        dnsinfo_win.cpp
        dnsinfo_win.h
        measurementcpuusage.cpp
        measurementcpuusage.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        ipv6controller_mac.cpp
        ipv6controller_mac.h
    )
elseif(UNIX)
    #todo linux
endif()


add_subdirectory(apiinfo)
add_subdirectory(apiresources)
add_subdirectory(autoupdater)
add_subdirectory(connectionmanager)
add_subdirectory(connectstatecontroller)
add_subdirectory(customconfigs)
add_subdirectory(dns_utils)
add_subdirectory(emergencycontroller)
add_subdirectory(firewall)
add_subdirectory(helper)
add_subdirectory(locationsmodel)
add_subdirectory(macaddresscontroller)
add_subdirectory(networkdetectionmanager)
add_subdirectory(ping)
add_subdirectory(proxy)
add_subdirectory(taputils)
add_subdirectory(vpnshare)
add_subdirectory(wireguardconfig)
