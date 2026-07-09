if(NOT TARGET wsnet::wsnet)
    include(FetchContent)
    FetchContent_Declare(wsnet
        GIT_REPOSITORY https://github.com/Windscribe/wsnet.git
        GIT_TAG        1.5.27
        #AAA for debug
        #SOURCE_DIR     "c:/work/windscribe/wsnet"
    )
    set(IS_BUILD_TESTS OFF)
    FetchContent_MakeAvailable(wsnet)
endif()
