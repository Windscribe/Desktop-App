#pragma once

#include <Security/Security.h>
#include <xpc/xpc.h>

namespace HelperSecurity {
    extern "C" bool isValidXpcConnection(xpc_object_t event);

    // After isValidXpcConnection() returns true, the validated caller's SecCodeRef
    // is stored thread-locally so the active command handler can resolve the calling
    // bundle's path (setInstallerPaths stages that bundle rather than trusting a path
    // from the wire). It attests to caller identity only: SecCodeCheckValidity does not
    // hash sealed resources, so it says nothing about the bundle's non-code contents.
    // Returns NULL when no validation has occurred on this thread yet, or after
    // clearCurrentCallerSecCode().
    //
    // Ownership remains with HelperSecurity; do not CFRelease.
    SecCodeRef currentCallerSecCode();
    void clearCurrentCallerSecCode();
}

