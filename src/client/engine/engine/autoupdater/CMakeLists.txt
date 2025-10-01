target_sources(engine PRIVATE
    downloadhelper.cpp
    downloadhelper.h
    # autoupdaterhelper_mac.cpp
    # autoupdaterhelper_mac.h
)


if(APPLE)
    target_sources(engine PRIVATE
        autoupdaterhelper_mac.cpp
        autoupdaterhelper_mac.h
    )
endif(APPLE)
