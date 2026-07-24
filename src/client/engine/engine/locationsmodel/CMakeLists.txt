target_sources(engine PRIVATE
    apilocationsmodel.cpp
    apilocationsmodel.h
    baselocationinfo.cpp
    baselocationinfo.h
    bestlocation.cpp
    bestlocation.h
    customconfiglocationinfo.cpp
    customconfiglocationinfo.h
    customconfiglocationsmodel.cpp
    customconfiglocationsmodel.h
    enginelocationsmodel.cpp
    enginelocationsmodel.h
    locationnode.h
    mutablelocationinfo.cpp
    mutablelocationinfo.h
    nodeselectionalgorithm.cpp
    nodeselectionalgorithm.h
)

if(IS_BUILD_TESTS)
    add_subdirectory(tests)
endif(IS_BUILD_TESTS)
