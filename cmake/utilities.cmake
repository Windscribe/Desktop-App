# ------------------------------------------------------------------------------
# Build System Utility Functions
# ------------------------------------------------------------------------------

# Resolve an output name template by replacing @VERSION@, @SUFFIX@, @ARCH@ placeholders.
# Empty placeholders are replaced with "", and resulting double/trailing underscores are cleaned up.
function(ws_resolve_output_name TEMPLATE OUT_VAR)
    set(_name "${TEMPLATE}")
    string(REPLACE "@VERSION@" "${PROJECT_VERSION}" _name "${_name}")
    string(REPLACE "@SUFFIX@" "${WS_BUILD_SUFFIX}" _name "${_name}")
    if(DEFINED PACKAGE_ARCH)
        string(REPLACE "@ARCH@" "${PACKAGE_ARCH}" _name "${_name}")
    else()
        string(REPLACE "@ARCH@" "" _name "${_name}")
    endif()
    # Clean up double underscores and trailing underscores from empty placeholders
    string(REGEX REPLACE "__+" "_" _name "${_name}")
    string(REGEX REPLACE "_$" "" _name "${_name}")
    set(${OUT_VAR} "${_name}" PARENT_SCOPE)
endfunction()


# Resolve bundled helper source paths.
# Populates WS_BUNDLED_HELPERS as a list of "source_path|dest_name" pairs used by packaging.
# Must be called after VCPKG_ROOT and WINDSCRIBE_BUILD_LIBS_PATH are set.
function(ws_resolve_bundled_helpers)
    set(_vcpkg_tools "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools")
    set(_build_libs  "${WINDSCRIBE_BUILD_LIBS_PATH}")
    set(_p           "${WS_PRODUCT_NAME_LOWER}")

    set(_SRC_openvpn_WIN      "${_vcpkg_tools}/openvpn/openvpn.exe")
    set(_SRC_openvpn_POSIX     "${_vcpkg_tools}/openvpn/sbin/openvpn")
    set(_SRC_ctrld_WIN        "${_vcpkg_tools}/ctrld/ctrld.exe")
    set(_SRC_ctrld_POSIX       "${_vcpkg_tools}/ctrld/ctrld")
    set(_SRC_wstunnel_WIN     "${_build_libs}/wstunnel/${_p}wstunnel.exe")
    set(_SRC_wstunnel_POSIX    "${_build_libs}/wstunnel/${_p}wstunnel")
    set(_SRC_amneziawg_POSIX   "${_build_libs}/wireguard/${_p}amneziawg")

    # amneziawg on Windows bundles raw DLLs instead of a single prefixed binary.
    set(_SRC_amneziawg_WIN_DLLS
        "${_build_libs}/wireguard/tunnel.dll|tunnel.dll"
        "${_build_libs}/wireguard/wireguard.dll|wireguard.dll"
        "${_build_libs}/wireguard/amneziawgtunnel.dll|amneziawgtunnel.dll"
    )

    set(_helpers "")
    foreach(_helper ${WS_BUNDLED_HELPER_NAMES})
        if(WIN32 AND DEFINED _SRC_${_helper}_WIN_DLLS)
            list(APPEND _helpers ${_SRC_${_helper}_WIN_DLLS})
            continue()
        endif()

        if(WIN32)
            set(_src "${_SRC_${_helper}_WIN}")
            set(_dest "${_p}${_helper}.exe")
        else()
            set(_src "${_SRC_${_helper}_POSIX}")
            set(_dest "${_p}${_helper}")
        endif()

        list(APPEND _helpers "${_src}|${_dest}")
    endforeach()
    set(WS_BUNDLED_HELPERS "${_helpers}" PARENT_SCOPE)
endfunction()

# Resolve shared library source paths.
# Populates WS_SHARED_LIBS as a list of "source_path|dest_name" pairs.
# Must be called after VCPKG_ROOT is set.
function(ws_resolve_shared_libs)
    set(_vcpkg_lib "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
    set(_libs "")

    foreach(_lib ${WS_SHARED_LIB_NAMES})
        if(_lib STREQUAL "wsnet")
            list(APPEND _libs "$<TARGET_FILE:wsnet>|$<TARGET_FILE_NAME:wsnet>")
        elseif(_lib STREQUAL "openssl")
            if(APPLE)
                list(APPEND _libs
                    "${_vcpkg_lib}/libssl.3.dylib|libssl.3.dylib"
                    "${_vcpkg_lib}/libcrypto.3.dylib|libcrypto.3.dylib")
            elseif(UNIX)
                list(APPEND _libs
                    "${_vcpkg_lib}/libssl.so.3|libssl.so.3"
                    "${_vcpkg_lib}/libcrypto.so.3|libcrypto.so.3")
            endif()
        else()
            message(FATAL_ERROR "Unknown shared library: ${_lib}")
        endif()
    endforeach()

    set(WS_SHARED_LIBS "${_libs}" PARENT_SCOPE)
endfunction()
