target_sources(frontend-common PRIVATE
    directorypermissions.h
)

if (WIN32)
    target_sources(frontend-common PRIVATE
        directorypermissions_win.cpp
        installedantiviruses_win.cpp
        installedantiviruses_win.h
    )
    target_link_libraries(frontend-common PRIVATE authz netapi32 shlwapi)
else()
    target_sources(frontend-common PRIVATE
        directorypermissions_posix.cpp
    )
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(frontend-common PRIVATE acl)
endif()
