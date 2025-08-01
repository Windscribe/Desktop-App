target_sources(wsnet PRIVATE
    cancelablecallback.h
    wsnet_callback_sink.h
    utils.h
    utils.cpp
    crypto_utils.cpp
    crypto_utils.h
    wsnet_logger.cpp
    wsnet_logger.h
    persistentsettings.cpp
    persistentsettings.h
    requesterror.cpp
    requesterror.h
    spdlog_utils.h
    urlquery_utils.cpp
    urlquery_utils.h
)

# Only include OQS provider files for mobile platforms
if(ANDROID OR IOS)
    target_sources(wsnet PRIVATE
        oqs_provider_loader.cpp
        oqs_provider_loader.h
    )
endif()
