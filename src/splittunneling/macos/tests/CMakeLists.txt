find_package(spdlog CONFIG REQUIRED)

add_executable(iphostnamesmanager.test
    iphostnamesmanager.test.cpp
    ../ip_hostnames/ares_library_init.cpp
    ../ip_hostnames/dns_resolver.cpp
    ../ip_hostnames/ip_hostnames_manager.cpp
    ../../../client/client-common/types/ipaddress.cpp
)
target_link_libraries(iphostnamesmanager.test PRIVATE c-ares::cares spdlog::spdlog)
set_target_properties(iphostnamesmanager.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
ws_add_test(iphostnamesmanager)

add_executable(udpsocket.test
    udpsocket.test.mm
    ../UdpSocket.mm
)
target_link_libraries(udpsocket.test PRIVATE "-framework Foundation")
set_target_properties(udpsocket.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
ws_add_test(udpsocket)

add_executable(flowudp.test
    flowudp.test.mm
    ../FlowUDP.mm
    ../UdpSocket.mm
    ../Utils.mm
)
target_link_libraries(flowudp.test PRIVATE
    spdlog::spdlog
    "-framework Foundation"
    "-framework Network"
    "-framework NetworkExtension"
)
set_target_properties(flowudp.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
ws_add_test(flowudp)
