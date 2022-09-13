target_sources(gui PRIVATE
    widgetutils.cpp
    widgetutils.h
)

if (WIN32)
    target_sources(gui PRIVATE
        widgetutils_win.cpp
    )
elseif(APPLE)
    target_sources(gui PRIVATE
        widgetutils_mac.h
        widgetutils_mac.mm
    )
endif()
