target_sources(gui PRIVATE
    authcheckerfactory.h
    iauthchecker.h
    imagewithshadow.cpp
    imagewithshadow.h
    interfaceutils.h
    makecustomshadow.cpp
    makecustomshadow.h
    shadowmanager.cpp
    shadowmanager.h
    textshadow.cpp
    textshadow.h
    writeaccessrightschecker.cpp
    writeaccessrightschecker.h
)

if (WIN32)
    target_sources(gui PRIVATE
        authchecker_win.cpp
        authchecker_win.h
        interfaceutils_win.cpp
        scaleutils_win.cpp
        scaleutils_win.h
    )
elseif(APPLE)
    target_sources(gui PRIVATE
        authchecker_mac.h
        authchecker_mac.mm
        interfaceutils_mac.mm
    )
elseif(UNIX)
    target_sources(gui PRIVATE
        authchecker_linux.cpp
        authchecker_linux.h
        interfaceutils_linux.cpp
    )
endif()
