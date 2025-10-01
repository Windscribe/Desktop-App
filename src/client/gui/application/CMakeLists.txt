target_sources(gui PRIVATE
    singleappinstance.cpp
    singleappinstance.h
    singleappinstance_p.h
    windscribeapplication.cpp
    windscribeapplication.h
)

if (WIN32)
    target_sources(gui PRIVATE
        windowsnativeeventfilter.cpp
        windowsnativeeventfilter.h
    )
elseif (APPLE)
    target_sources(gui PRIVATE
        checkrunningapp/checkrunningapp_mac.mm
        checkrunningapp/checkrunningapp_mac.h
    )
endif()
