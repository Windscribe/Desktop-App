target_sources(common PRIVATE
    clean_sensitive_info.cpp
    clean_sensitive_info.h
    dga_library.cpp
    dga_library.h
    dga_parameters.h 
    executable_signature/executable_signature.cpp
    executable_signature/executable_signature.h
    extraconfig.cpp
    extraconfig.h
    hardcodedsettings.cpp
    hardcodedsettings.h
    ipvalidation.cpp
    ipvalidation.h
    languagesutil.cpp
    languagesutil.h
    logger.cpp
    logger.h
    mergelog.cpp
    mergelog.h
    multiline_message_logger.h
    simplecrypt.cpp
    simplecrypt.h
    utils.cpp
    utils.h
    ws_assert.h
)

if (WIN32)
    target_sources(common PRIVATE
        crashdump.cpp
        crashhandler.cpp
        winutils.cpp
        executable_signature/executable_signature_win.cpp
        servicecontrolmanager.cpp
    )
elseif(APPLE)
    target_sources(common PRIVATE
        exithandler_mac.mm
        exithandler_mac.h
        executable_signature/executable_signature_mac.h
        executable_signature/executable_signature_mac.mm
        macutils.h
        macutils.mm
        network_utils/network_utils_mac.cpp
        network_utils/network_utils_mac.h
    )
elseif(UNIX)
    target_sources(common PRIVATE
        executable_signature/executablesignature_linux.cpp
        executable_signature/executablesignature_linux.h
        linuxutils.cpp
        linuxutils.h
    )
endif()
