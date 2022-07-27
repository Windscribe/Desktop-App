target_sources(gui PRIVATE
    authcheckerfactory.h
    iauthchecker.h
    imagewithshadow.cpp
    imagewithshadow.h
    interfaceutils.h
    makecustomshadow.cpp
    makecustomshadow.h
    protoenumtostring.cpp
    protoenumtostring.h
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
endif (WIN32)
