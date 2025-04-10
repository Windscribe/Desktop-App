target_sources(base PRIVATE
    proxymodels/cities_proxymodel.cpp
    proxymodels/cities_proxymodel.h
    proxymodels/customconfigs_proxymodel.cpp
    proxymodels/customconfigs_proxymodel.h
    proxymodels/favoritecities_proxymodel.cpp
    proxymodels/favoritecities_proxymodel.h
    proxymodels/sortedcities_proxymodel.cpp
    proxymodels/sortedcities_proxymodel.h
    proxymodels/sortedlocations_proxymodel.cpp
    proxymodels/sortedlocations_proxymodel.h
    proxymodels/staticips_proxymodel.cpp
    proxymodels/staticips_proxymodel.h
    favoritelocationsstorage.cpp
    favoritelocationsstorage.h
    locationsmodel.cpp
    locationsmodel.h
    locationsmodel_utils.cpp
    locationsmodel_utils.h
    locationitem.cpp
    locationitem.h
    renamedlocationsstorage.cpp
    renamedlocationsstorage.h
    selectedlocation.cpp
    selectedlocation.h
)

# unit tests
if(DEFINED IS_BUILD_TESTS)
    set(TEST_SOURCES
        locationsmodel.test.cpp
        locationsmodel.test.h
        locationsmodel.test.qrc
    )

    add_executable (locationsmodel.test ${TEST_SOURCES})
    target_link_libraries(locationsmodel.test PRIVATE Qt6::Test base engine common spdlog::spdlog ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(locationsmodel.test PRIVATE
        ${PROJECT_DIRECTORY}/base
        ${PROJECT_DIRECTORY}/common
    )
    set_target_properties(locationsmodel.test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

endif(DEFINED IS_BUILD_TESTS)
