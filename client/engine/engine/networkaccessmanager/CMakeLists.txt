target_sources(engine PRIVATE
    certmanager.cpp
    certmanager.h
    curlinitcontroller.cpp
    curlinitcontroller.h
    curlnetworkmanager.cpp
    curlnetworkmanager.h
    curlreply.cpp
    curlreply.h
    dnscache.cpp
    dnscache.h
    networkaccessmanager.cpp
    networkaccessmanager.h
    networkreply.cpp
    networkreply.h
    networkrequest.cpp
    networkrequest.h
)

if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)

