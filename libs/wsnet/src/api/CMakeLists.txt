target_sources(wsnet PRIVATE
    baserequest.cpp
    baserequest.h
)

add_subdirectory(bridgeapi)
add_subdirectory(serverapi)
