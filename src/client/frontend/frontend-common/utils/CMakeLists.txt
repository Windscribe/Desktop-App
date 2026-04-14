target_sources(frontend-common PRIVATE
    authcheckerfactory.h
    iauthchecker.h
    writeaccessrightschecker.cpp
    writeaccessrightschecker.h
)

if (WIN32)
    target_sources(frontend-common PRIVATE
        authchecker_win.cpp
        authchecker_win.h
        installedantiviruses_win.cpp
        installedantiviruses_win.h
    )
elseif(APPLE)
    target_sources(frontend-common PRIVATE
        authchecker_mac.h
        authchecker_mac.mm
    )
elseif(UNIX)
    target_sources(frontend-common PRIVATE
        authchecker_linux.cpp
        authchecker_linux.h
    )
endif()
