target_sources(gui PRIVATE
    locations.cpp
    locations.h
    locationsmodel_roles.h
    model/citiesmodel.cpp
    model/citiesmodel.h
    model/customconfigsmodel.cpp
    model/customconfigsmodel.h
    model/favoritecitiesmodel.cpp
    model/favoritecitiesmodel.h
    model/favoritelocationsstorage.cpp
    model/favoritelocationsstorage.h
    model/locationsmodel.cpp
    model/locationsmodel.h
    model/selectedlocationwatcher.cpp
    model/selectedlocationwatcher.h
    model/sortedcitiesmodel.cpp
    model/sortedcitiesmodel.h
    model/sortedlocationsmodel.cpp
    model/sortedlocationsmodel.h
    model/staticipsmodel.cpp
    model/staticipsmodel.h
    view/cityitemdelegate.cpp
    view/cityitemdelegate.h
    view/clickableandtooltiprects.h
    view/countryitemdelegate.cpp
    view/countryitemdelegate.h
    view/cursorupdatehelper.cpp
    view/cursorupdatehelper.h
    view/expandableitemswidget.cpp
    view/expandableitemswidget.h
    view/iitemdelegate.h
    view/locationsview.cpp
    view/locationsview.h
)

# unit tests
if(DEFINED IS_BUILD_TESTS)

    set(TEST_SOURCES
        model/locationsmodel.test.cpp
        model/locationsmodel.test.h
        model/locationsmodeldata.test.cpp
        model/locationsmodeldata.test.h
    )

    add_executable (locationsmodel.test ${TEST_SOURCES})
    target_link_libraries(locationsmodel.test PRIVATE Qt6::Test gui engine common ${OS_SPECIFIC_LIBRARIES})
    target_include_directories(locationsmodel.test PRIVATE
        ${PROJECT_DIRECTORY}/engine
        ${PROJECT_DIRECTORY}/gui
        ${PROJECT_DIRECTORY}/../common
    )
    set_target_properties( locationsmodel.test
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endif(DEFINED IS_BUILD_TESTS)



