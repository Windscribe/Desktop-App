target_sources(engine PRIVATE
    firewallcontroller.cpp
    firewallcontroller.h
    firewallexceptions.cpp
    firewallexceptions.h
    uniqueiplist.cpp
    uniqueiplist.h
)

if (WIN32)
    target_sources(engine PRIVATE
        firewallcontroller_win.cpp
        firewallcontroller_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        firewallcontroller_mac.cpp
        firewallcontroller_mac.h
    )

elseif(UNIX)
    #todo linux
endif()
