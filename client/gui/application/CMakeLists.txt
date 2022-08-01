target_sources(gui PRIVATE
    singleappinstance.cpp
    singleappinstance.h
    singleappinstance_p.h
    windowsnativeeventfilter.cpp
    windowsnativeeventfilter.h
    windscribeapplication.cpp
    windscribeapplication.h
)

if(APPLE)
    target_sources(gui PRIVATE
        checkrunningapp/checkrunningapp_mac.mm
        checkrunningapp/checkrunningapp_mac.h
    )
endif(APPLE)
