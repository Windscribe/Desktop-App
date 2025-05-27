cmake_minimum_required(VERSION 3.21)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12" CACHE STRING "Minimum OS X deployment version")
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

if (VCPKG_TARGET_ANDROID)
    include("cmake/vcpkg_android.cmake")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
##TODO:
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)
#set(CMAKE_VISIBILITY_INLINES_HIDDEN True)

if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        set(VCPKG_TARGET_TRIPLET arm64-windows-static)
    else()
        set(VCPKG_TARGET_TRIPLET x64-windows-static)
    endif()
endif()

project(wsnet
    DESCRIPTION "The wsnet library for Windscribe client programs"
    LANGUAGES CXX
)

find_package(c-ares CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(skyr-url CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(CMakeRC)
find_path(CPP_BASE64_INCLUDE_DIRS "cpp-base64/base64.cpp")

find_package(Boost REQUIRED COMPONENTS filesystem)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(STATUS "Boost NOT Found !")
endif(Boost_FOUND)

find_path(ADVOBFUSCATOR_INCLUDE_DIRS "Lib/Indexes.h")

# Get all public headers
# Each public header file must have one class and the file name must match the class name (Java language requirement).
# The file name must begin with the prefix "WSNet".
file(GLOB WS_CPP_PUBLIC_HEADERS RELATIVE ${PROJECT_SOURCE_DIR}  ${PROJECT_SOURCE_DIR}/include/wsnet/WSNet*.h)

cmrc_add_resource_library(
    cert-resources
    ALIAS wsnet::rc
    NAMESPACE wsnet
    resources/certs_bundle.pem
    resources/windscribe_cert.crt
    resources/emergency.ovpn
)
set_property(TARGET cert-resources PROPERTY POSITION_INDEPENDENT_CODE ON)

if (WIN32)
    add_library(wsnet STATIC ${WS_CPP_PUBLIC_HEADERS})
else()
    add_library(wsnet SHARED ${WS_CPP_PUBLIC_HEADERS})
endif()
add_library(wsnet::wsnet ALIAS wsnet)

target_compile_features(wsnet PUBLIC cxx_std_17)

if (DEFINED IS_BUILD_TESTS)
    if (NOT WIN32)
        target_compile_options(wsnet PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
        target_link_options(wsnet PRIVATE --coverage)
    endif()
endif()

# Set platform specific dependencies
if (IOS)
    set (OS_SPECIFIC_LIBRARIES "-framework Foundation")
endif()

target_link_libraries(wsnet PRIVATE c-ares::cares CURL::libcurl spdlog::spdlog rapidjson skyr::skyr-url OpenSSL::SSL wsnet::rc Boost::filesystem ${OS_SPECIFIC_LIBRARIES})
target_include_directories(wsnet PRIVATE
    ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/wsnet ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CPP_BASE64_INCLUDE_DIRS} ${ADVOBFUSCATOR_INCLUDE_DIRS}
)

if (WIN32)
    target_compile_definitions(wsnet PRIVATE CMAKE_LIBRARY_LIBRARY
                                  WINVER=0x0601
                                  _WIN32_WINNT=0x0601
                                  WIN32_LEAN_AND_MEAN
                                  PIO_APC_ROUTINE_DEFINED)

    set_target_properties(wsnet PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif (WIN32)

target_include_directories(wsnet PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# let the preprocessor know about Apple tvOS
if(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
  target_compile_definitions(wsnet PUBLIC "IS_TVOS")
endif()

# For Android and iOS using the scapix library for automatic binding to Java/Objective C languages
if(ANDROID OR IOS)
    target_compile_definitions(wsnet PUBLIC "IS_MOBILE_PLATFORM")
    find_package(scapix CONFIG REQUIRED)
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

    # This file is also used by the above generated headers, so copy it to framework headers
    install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/src/scapix/source/scapix/bridge/objc/BridgeObject.h
        DESTINATION wsnet.framework/Headers/scapix/bridge/objc
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
