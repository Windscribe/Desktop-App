if(NOT TARGET wsnet::wsnet)
    include(FetchContent)
    FetchContent_Declare(wsnet
        GIT_REPOSITORY https://github.com/Windscribe/wsnet.git
        GIT_TAG        1.5.6.3
    )
    FetchContent_MakeAvailable(wsnet)
endif()
