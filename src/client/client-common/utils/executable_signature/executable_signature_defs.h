#pragma once

// This should match the 'Name of signer' that appears in the Digital Signatures page of the
// signed executable's file properties dialog.
#define WINDOWS_CERT_SUBJECT_NAME L"Windscribe Limited"

// The Apple Developer Team ID (the subject.OU of the Developer ID Application certificate).
// Runtime code-signing requirement strings pin this rather than the CN (which embeds the organization
// name and would change on a legal-entity rename), and cmake/signing.cmake extracts it as
// DEVELOPMENT_TEAM for codesigning, plist substitution, and entitlements.
#define MACOS_CERT_TEAM_ID "GYZJYS7XUG"

// Codesigning requirement shared by the client-side and helper-side verifiers: a genuine Apple-anchored
// Developer ID chain (marker OIDs) with the leaf pinned to Windscribe's team ID.
#define MACOS_DEVID_REQUIREMENT "anchor apple generic and certificate 1[field.1.2.840.113635.100.6.2.6] and " \
                                "certificate leaf[field.1.2.840.113635.100.6.1.13] and " \
                                "certificate leaf[subject.OU] = \"" MACOS_CERT_TEAM_ID "\""

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
