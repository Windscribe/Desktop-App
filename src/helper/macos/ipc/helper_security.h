#pragma once

#include <Security/Security.h>
#include <xpc/xpc.h>

namespace HelperSecurity {
    extern "C" bool isValidXpcConnection(xpc_object_t event);

    // After isValidXpcConnection() returns true, the validated caller's SecCodeRef
    // is stored thread-locally so the active command handler can re-verify the
    // caller's signature (e.g. during file installation, to anchor archive reads
    // to the verified bundle). Returns NULL when no validation has occurred on
    // this thread yet, or after clearCurrentCallerSecCode().
    //
    // Ownership remains with HelperSecurity; do not CFRelease.
    SecCodeRef currentCallerSecCode();
    void clearCurrentCallerSecCode();

    // Re-validate the current caller's SecCode against the same requirement
    // as the original handshake. Use this after any operation that may have
    // observed bytes from the caller's bundle on disk (e.g. reading an archive
    // resource), to detect on-disk tampering between handshake and read.
    bool recheckCurrentCaller();
}

