target_include_directories(wsnet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(wsnet PRIVATE
    basefailover.h
    failoverdata.h
    failovers/hardcodeddomainfailover.h
    ifailovercontainer.h
)

