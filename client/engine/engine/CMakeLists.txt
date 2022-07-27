target_sources(engine PRIVATE
    crossplatformobjectfactory.cpp
    crossplatformobjectfactory.h
    engine.cpp
    enginesettings.cpp
    getdeviceid.cpp
    getmyipcontroller.cpp
    packetsizecontroller.cpp
    refetchservercredentialshelper.cpp
    sessionstatustimer.cpp
    openvpnversioncontroller.cpp
    engine.h
    enginesettings.h
    getdeviceid.h
    getmyipcontroller.h
    openvpnversioncontroller.h
    packetsizecontroller.h
    refetchservercredentialshelper.h
    sessionstatustimer.h

    #engine/tempscripts_mac.cpp
    #engine/tempscripts_mac.h
    #engine/ipv6controller_mac.cpp
    #engine/ipv6controller_mac.h
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
endif (WIN32)

add_subdirectory(apiinfo)
add_subdirectory(autoupdater)
add_subdirectory(connectionmanager)
add_subdirectory(connectstatecontroller)
add_subdirectory(customconfigs)
add_subdirectory(dnsresolver)
add_subdirectory(emergencycontroller)
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
add_subdirectory(types)
add_subdirectory(vpnshare)
add_subdirectory(wireguardconfig)

