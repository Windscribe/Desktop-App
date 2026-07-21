target_sources(engine PRIVATE
    dnsconfigurator.cpp
    dnsconfigurator.h
    idnsconfigurator.h
)

if (APPLE)
    target_sources(engine PRIVATE
        restorednsmanager_mac.h
        restorednsmanager_mac.mm
    )
endif()

add_subdirectory(ctrldmanager)
