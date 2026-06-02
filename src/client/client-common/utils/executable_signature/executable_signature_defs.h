#pragma once

// This should match the 'Name of signer' that appears in the Digital Signatures page of the
// signed executable's file properties dialog.
#define WINDOWS_CERT_SUBJECT_NAME L"Windscribe Limited"

// This should match the 'Name' displayed for your Developer ID Application certificate in Keychain Access.
#define MACOS_CERT_DEVELOPER_ID "Developer ID Application: Windscribe Limited (GYZJYS7XUG)"

// Accepted V4 fingerprints (40 hex chars, uppercase, no spaces) of the Windscribe Linux Packaging
// master keys. The helper rejects any in-app update whose downloaded .pub does not contain a primary
// key matching one of these entries. List form supports master rotation: a new master is appended in
// release N, the old one stays valid until a future release drops it.
//
// The trust anchor is the master fingerprint, not a hash of the .pub bytes — that lets us rotate the
// signing subkey (the one held in CI) without requiring a client release. gpgv and librpm validate the
// master->subkey binding signature inside the .pub natively.
#define LINUX_SIGNING_MASTER_FINGERPRINTS \
    { "441B49B9D5AFCCAC158444F4E699B988472B0781" }
