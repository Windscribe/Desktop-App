# ------------------------------------------------------------------------------
# Configure a single dependency template file with integration-specific values.
# This is used to configure dependencies in tools/deps with custom branding.
#
# Invoked as:
#   cmake -DPRODUCT=windscribe -DINPUT_FILE=<template.in> -DOUTPUT_FILE=<output>
#         -P cmake/configure_dependency.cmake
# ------------------------------------------------------------------------------

if(NOT DEFINED PRODUCT)
    message(FATAL_ERROR "PRODUCT is not defined")
endif()
if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is not defined")
endif()
if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE is not defined")
endif()

set(WS_INTEGRATION_CONFIG "${CMAKE_CURRENT_LIST_DIR}/integrations/${PRODUCT}.cmake")
if(NOT EXISTS "${WS_INTEGRATION_CONFIG}")
    message(FATAL_ERROR "Integration config not found: ${WS_INTEGRATION_CONFIG}")
endif()
include("${WS_INTEGRATION_CONFIG}")

configure_file("${INPUT_FILE}" "${OUTPUT_FILE}" @ONLY)
