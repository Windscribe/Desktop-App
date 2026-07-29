# ------------------------------------------------------------------------------
# Build System Utility Functions
# ------------------------------------------------------------------------------

# Register a Qt test executable with CTest under the given name.
# Expects an existing target named "${name}.test".
function(ws_add_test name)
    add_test(NAME ${name} COMMAND ${name}.test)
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "client-desktop"
    )
endfunction()


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


# Resolve the version of the bundled OpenVPN. Sent as ovpn_version on the ServerConfigs request, and
# the server tailors the returned config to it. Asking vcpkg tracks whatever the registry pins and
# works when cross-compiling. Call after VCPKG_ROOT is set and the packages are installed.
function(ws_resolve_openvpn_version OUT_VAR)
    # `list` reports only what is installed, for this triplet. Parsing the status database by hand
    # would have to reproduce its rules: removed ports keep their stanza, features get their own.
    # --x-install-root must match the one install_vcpkg_dependencies() installs into.
    execute_process(
        COMMAND "${VCPKG_ROOT}/vcpkg" list openvpn --x-json
                "--x-install-root=${VCPKG_ROOT}/installed"
        WORKING_DIRECTORY "${VCPKG_ROOT}"
        OUTPUT_VARIABLE _json
        ERROR_VARIABLE _listError
        RESULT_VARIABLE _listResult
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _listResult EQUAL 0)
        message(FATAL_ERROR "ws_resolve_openvpn_version: vcpkg list failed (${_listResult}): ${_listError}")
    endif()

    # Entries are keyed "<port>:<triplet>". The port version (a packaging revision) is deliberately
    # left off: the API wants upstream's version.
    string(JSON _ver ERROR_VARIABLE _jsonError
           GET "${_json}" "openvpn:${VCPKG_TARGET_TRIPLET}" "version")

    # Fail rather than emit an empty or malformed string: a wrong ovpn_version would silently get us
    # configs for the wrong OpenVPN, which is exactly the drift this is meant to prevent.
    if(_jsonError OR NOT _ver MATCHES "^[0-9]+\\.[0-9]+")
        message(FATAL_ERROR "ws_resolve_openvpn_version: no usable openvpn version for triplet "
                            "${VCPKG_TARGET_TRIPLET} (error: ${_jsonError}, value: \"${_ver}\")")
    endif()

    set(${OUT_VAR} "${_ver}" PARENT_SCOPE)
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

# Populate WS_OPENSSL_SSL_LIB and WS_OPENSSL_CRYPTO_LIB with the installed library file names. The
# installed artifact is matched rather than the name rebuilt from the port version, because the SONAME
# does not track it: openssl 1.1.1 shipped SONAME 1.1, the pinned 4.0.1 ships 4.
function(ws_resolve_openssl_libs)
    set(_vcpkg_lib "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
    foreach(_stem ssl crypto)
        if(APPLE)
            # Two literal dots, so the unversioned lib<stem>.dylib symlink is not a candidate.
            file(GLOB _found RELATIVE "${_vcpkg_lib}" "${_vcpkg_lib}/lib${_stem}.*.dylib")
        else()
            file(GLOB _found RELATIVE "${_vcpkg_lib}" "${_vcpkg_lib}/lib${_stem}.so.*")
        endif()

        if(NOT _found)
            message(FATAL_ERROR "ws_resolve_openssl_libs: no lib${_stem} in ${_vcpkg_lib}")
        endif()

        # The SONAME, which is the shortest candidate: a fully versioned libssl.so.4.0.1 may sit
        # beside libssl.so.4, and the fully versioned name is the SONAME plus further digits.
        set(_soname "")
        set(_shortest 0)
        foreach(_candidate IN LISTS _found)
            string(LENGTH "${_candidate}" _len)
            if(_soname STREQUAL "" OR _len LESS _shortest)
                set(_soname "${_candidate}")
                set(_shortest ${_len})
            endif()
        endforeach()

        string(TOUPPER ${_stem} _stem_upper)
        set(WS_OPENSSL_${_stem_upper}_LIB "${_soname}" PARENT_SCOPE)
    endforeach()
endfunction()

# Resolve shared library source paths.
# Populates WS_SHARED_LIBS as a list of "source_path|dest_name" pairs.
# Must be called after VCPKG_ROOT and ws_resolve_openssl_libs() are set.
function(ws_resolve_shared_libs)
    set(_vcpkg_lib "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
    set(_libs "")

    foreach(_lib ${WS_SHARED_LIB_NAMES})
        if(_lib STREQUAL "wsnet")
            list(APPEND _libs "$<TARGET_FILE:wsnet>|$<TARGET_FILE_NAME:wsnet>")
        elseif(_lib STREQUAL "openssl")
            # Only once the names are resolved. A configure with no vcpkg tree (signing only) never
            # consumes WS_SHARED_LIBS, and must not have a malformed pair built for it here.
            if((APPLE OR UNIX) AND WS_OPENSSL_SSL_LIB AND WS_OPENSSL_CRYPTO_LIB)
                list(APPEND _libs
                    "${_vcpkg_lib}/${WS_OPENSSL_SSL_LIB}|${WS_OPENSSL_SSL_LIB}"
                    "${_vcpkg_lib}/${WS_OPENSSL_CRYPTO_LIB}|${WS_OPENSSL_CRYPTO_LIB}")
            endif()
        else()
            message(FATAL_ERROR "Unknown shared library: ${_lib}")
        endif()
    endforeach()

    set(WS_SHARED_LIBS "${_libs}" PARENT_SCOPE)
endfunction()
