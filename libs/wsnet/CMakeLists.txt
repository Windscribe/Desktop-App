cmake_minimum_required(VERSION 3.21)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11" CACHE STRING "Minimum OS X deployment version")
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

set(wsnet_VERSION 1.4.10)

if (VCPKG_TARGET_ANDROID)
    include("cmake/vcpkg_android.cmake")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
##TODO:
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)
#set(CMAKE_VISIBILITY_INLINES_HIDDEN True)

project(wsnet VERSION ${wsnet_VERSION}
    DESCRIPTION "The wsnet library for Windscribe client programs"
    HOMEPAGE_URL "https://gitlab.int.windscribe.com/ws/client/client-libs/wsnet.git"
    LANGUAGES CXX
)

#generate a version header file
configure_file(wsnet_version.h.in wsnet_version.h)

# Helpers functions for creating config files that can be included by other projects to find and use a package
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

#include code coverage cmake tool
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;")
include(code-coverage)
add_code_coverage_all_targets()

#TODO: move fork to another repo or make vcpkg port?
# For Android and iOS using the scapix library for automatic binding to Java/Objective C languages
if(ANDROID OR IOS)
    include(FetchContent)
    FetchContent_Declare(
      cmodule
      URL "https://github.com/AzanovAA/cmodule/archive/refs/heads/master.zip"
      URL_HASH SHA256=895b6676f81b487bff254eb9e0550e86b1325b84595973205056531e4831d3f8

      #URL "https://github.com/scapix-com/cmodule/archive/refs/tags/v1.0.42.tar.gz"
      #URL_HASH SHA256=0fc5bb6bde3054664713bc9cba48f44a2f8af8fa2b94eb0da1031d5298f9ccf6
    )
    FetchContent_MakeAvailable(cmodule)
    find_package(Scapix REQUIRED)
endif()

find_package(c-ares CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(skyr-url CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(CMakeRC)
find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
find_path(CPP_BASE64_INCLUDE_DIRS "cpp-base64/base64.cpp")
find_package(reproc++ CONFIG REQUIRED)

find_path(ADVOBFUSCATOR_INCLUDE_DIRS "Lib/Indexes.h")

# Each public header file must have one class and the file name must match the class name (Java language requirement).
set(WS_CPP_PUBLIC_HEADERS
    include/wsnet/WSNet.h
    include/wsnet/WSNetAdvancedParameters.h
    include/wsnet/WSNetCancelableCallback.h
    include/wsnet/WSNetDnsRequestResult.h
    include/wsnet/WSNetDnsResolver.h
    include/wsnet/WSNetEmergencyConnect.h
    include/wsnet/WSNetEmergencyConnectEndpoint.h
    include/wsnet/WSNetHttpRequest.h
    include/wsnet/WSNetHttpNetworkManager.h
    include/wsnet/WSNetServerAPI.h
    include/wsnet/WSNetPingManager.h
    include/wsnet/WSNetUtils.h
)

cmrc_add_resource_library(
    cert-resources
    ALIAS wsnet::rc
    NAMESPACE wsnet
    resources/certs_bundle.pem
    resources/windscribe_cert.crt
    resources/emergency.ovpn
)
set_property(TARGET cert-resources PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(wsnet SHARED ${WS_CPP_PUBLIC_HEADERS})
add_library(wsnet::wsnet ALIAS wsnet)

target_code_coverage(wsnet AUTO ALL)

target_compile_features(wsnet PUBLIC cxx_std_17)

# Set platform specificd dependencies
if (IOS)
    set (OS_SPECIFIC_LIBRARIES "-framework Foundation")
endif()

target_link_libraries(wsnet PRIVATE c-ares::cares CURL::libcurl spdlog::spdlog rapidjson skyr::skyr-url OpenSSL::SSL wsnet::rc reproc++ ${OS_SPECIFIC_LIBRARIES})
target_include_directories(wsnet PRIVATE
    ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/wsnet ${CMAKE_CURRENT_SOURCE_DIR}/src ${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS}
    ${CPP_BASE64_INCLUDE_DIRS} ${ADVOBFUSCATOR_INCLUDE_DIRS}
)

if (WIN32)
    target_compile_definitions(wsnet PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)
endif (WIN32)

target_include_directories(wsnet PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


if(ANDROID OR IOS)
    scapix_bridge_headers(wsnet "com.wsnet.lib" ${WS_CPP_PUBLIC_HEADERS})
endif()

if(ANDROID)
    target_compile_definitions(wsnet PUBLIC SCAPIX_CUSTOM_JNI_ONLOAD SCAPIX_CACHE_CLASS_LOADER SCAPIX_JAVA_AUTO_ATTACH_THREAD)
endif()

if (IOS)
    foreach (CPP_HEADER ${WS_CPP_PUBLIC_HEADERS})
        string(REGEX REPLACE "include/wsnet" "generated/bridge/objc/lib/bridge" OBJ ${CPP_HEADER})
        set(WS_OBJC_PUBLIC_HEADERS ${WS_OBJC_PUBLIC_HEADERS} ${OBJ})
    endforeach ()

    set_target_properties(wsnet PROPERTIES
      FRAMEWORK TRUE
      MACOSX_FRAMEWORK_IDENTIFIER com.cmake.wsnet
      PUBLIC_HEADER "${WS_OBJC_PUBLIC_HEADERS}"
    )
endif()

# Strip binary for release builds (in particular for Android for some reason this is not done automatically)
if(ANDROID)
    add_custom_command(
      TARGET "${CMAKE_PROJECT_NAME}" POST_BUILD
      DEPENDS "${CMAKE_PROJECT_NAME}"
      COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
      ARGS --strip-all $<TARGET_FILE:${CMAKE_PROJECT_NAME}>)
else()
  ##TODO:
  #add_custom_command(
  #  TARGET "${CMAKE_PROJECT_NAME}" POST_BUILD
  #  DEPENDS "${CMAKE_PROJECT_NAME}"
  #  COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
  #  ARGS -u $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
  #)
endif()

if(ANDROID OR IOS)
    install(TARGETS wsnet
        LIBRARY DESTINATION .
        FRAMEWORK DESTINATION .
    )
else()
    #install(TARGETS wsnet EXPORT wsnet-targets)
endif()

add_subdirectory(src)
