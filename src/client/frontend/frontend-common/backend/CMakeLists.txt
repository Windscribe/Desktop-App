target_sources(frontend-common PRIVATE
    backend.cpp
    backend.h
    connectstatehelper.cpp
    connectstatehelper.h
    firewallstatehelper.cpp
    firewallstatehelper.h
    notificationscontroller.cpp
    notificationscontroller.h
    persistentstate.cpp
    persistentstate.h
    preferences/accountinfo.cpp
    preferences/accountinfo.h
    preferences/detectlanrange.cpp
    preferences/detectlanrange.h
    preferences/preferences.cpp
    preferences/preferences.h
    preferences/preferenceshelper.cpp
    preferences/preferenceshelper.h
    types/upgrademodetype.cpp
    types/upgrademodetype.h
)

# INI-backed preferences are only used by the CLI build.
if(FRONTEND_TYPE STREQUAL "cli")
    target_sources(frontend-common PRIVATE
        iniwatcher.cpp
        iniwatcher.h
    )
endif()
