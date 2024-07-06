target_sources(gui PRIVATE
    imagewithshadow.cpp
    imagewithshadow.h
    interfaceutils.h
    makecustomshadow.cpp
    makecustomshadow.h
    shadowmanager.cpp
    shadowmanager.h
    textshadow.cpp
    textshadow.h
)

if (WIN32)
    target_sources(gui PRIVATE
        interfaceutils_win.cpp
        scaleutils_win.cpp
        scaleutils_win.h
    )
elseif(APPLE)
    target_sources(gui PRIVATE
        interfaceutils_mac.mm
    )
elseif(UNIX)
    target_sources(gui PRIVATE
        interfaceutils_linux.cpp
    )
endif()
