target_sources(gui PRIVATE
    imagewithshadow.cpp
    imagewithshadow.h
    makecustomshadow.cpp
    makecustomshadow.h
    shadowmanager.cpp
    shadowmanager.h
    textshadow.cpp
    textshadow.h
)

if (WIN32)
    target_sources(gui PRIVATE
        scaleutils_win.cpp
        scaleutils_win.h
    )
endif()
