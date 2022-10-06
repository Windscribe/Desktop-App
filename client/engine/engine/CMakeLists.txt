target_sources(engine PRIVATE
    crossplatformobjectfactory.cpp
    crossplatformobjectfactory.h
    engine.cpp
    engine.h
    getdeviceid.cpp
    getdeviceid.h
    getmyipcontroller.cpp
    getmyipcontroller.h
    openvpnversioncontroller.cpp
    openvpnversioncontroller.h
    packetsizecontroller.cpp
    packetsizecontroller.h
    refetchservercredentialshelper.cpp
    refetchservercredentialshelper.h
    sessionstatustimer.cpp
    sessionstatustimer.h
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
        tempscripts_mac.cpp
        tempscripts_mac.h
    )
elseif(UNIX)
    #todo linux
endif()


add_subdirectory(apiinfo)
add_subdirectory(apiresourcesmanager)
add_subdirectory(autoupdater)
add_subdirectory(connectionmanager)
add_subdirectory(connectstatecontroller)
add_subdirectory(customconfigs)
add_subdirectory(dnsresolver)
add_subdirectory(emergencycontroller)
add_subdirectory(failover)
add_subdirectory(firewall)
add_subdirectory(helper)
add_subdirectory(locationsmodel)
add_subdirectory(logincontroller)
add_subdirectory(macaddresscontroller)
add_subdirectory(networkaccessmanager)
add_subdirectory(networkdetectionmanager)
add_subdirectory(ping)
add_subdirectory(proxy)
add_subdirectory(serverapi)
add_subdirectory(taputils)
add_subdirectory(utils)
add_subdirectory(vpnshare)
add_subdirectory(wireguardconfig)
