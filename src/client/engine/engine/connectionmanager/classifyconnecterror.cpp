#include "classifyconnecterror.h"

ConnectErrorClassification classifyConnectError(ConnectError err, bool isAutomaticMode, bool emitAuthError, bool retryOnAttemptFailure)
{
    switch (err) {
        case ConnectError::kAuthFailure:
            return emitAuthError ? ConnectErrorClassification::ErrorAfterDisconnect : ConnectErrorClassification::Retry;
        // An endpoint-list policy treats these local fatal errors as walk-to-the-next-endpoint;
        // otherwise they are attempt-fatal. A missing adapter driver (OpenVPN TAP) is reported only
        // after the non-TAP protocols in the automatic walk, so every remaining attempt needs the
        // same absent driver and failing over cannot help.
        case ConnectError::kLocalProcessNotResponding:
        case ConnectError::kAdapterNotInstalled:
            return retryOnAttemptFailure ? ConnectErrorClassification::Retry : ConnectErrorClassification::ErrorAfterDisconnect;
        case ConnectError::kLocalProcessLaunchFailure:
            return retryOnAttemptFailure ? ConnectErrorClassification::Retry : ConnectErrorClassification::ErrorImmediately;
        case ConnectError::kAdapterSetupFailure:
            if (retryOnAttemptFailure) {
                return ConnectErrorClassification::Retry;
            }
            [[fallthrough]];
        // Automatic mode fails over to another protocol; manual mode surfaces the failure.
        case ConnectError::kTunnelEstablishmentFailure:
            return isAutomaticMode ? ConnectErrorClassification::Retry : ConnectErrorClassification::ErrorAfterDisconnect;
        case ConnectError::kPrivKeyPasswordFailure:
            return ConnectErrorClassification::ErrorAfterDisconnect;
        case ConnectError::kVpnServiceSetupFailure:
        case ConnectError::kHostsFileNotWritable:
            return isAutomaticMode ? ConnectErrorClassification::Retry : ConnectErrorClassification::ErrorImmediately;
        case ConnectError::kTransientTunnelFailure:
            return ConnectErrorClassification::Retry;
        // Not runtime connector errors: these arrive via classifyPrepareError or engine-level paths.
        case ConnectError::kNoError:
        case ConnectError::kLocationUnavailable:
        case ConnectError::kAccountBlocked:
        case ConnectError::kCustomConfigInvalid:
        case ConnectError::kConfigFetchFailure:
        case ConnectError::kLocalConfigGenerationFailure:
        case ConnectError::kDnsServiceStartFailure:
        case ConnectError::kBlockedByOsPolicy:
        case ConnectError::kLocalDnsServerNotAvailable:
            return ConnectErrorClassification::Unknown;
    }
    return ConnectErrorClassification::Unknown;
}

// Routing for pre-dial prepare failures; runtime connector errors go through classifyConnectError.
// Config-acquisition failures route to failover rather than a hard stop: kLocalConfigGenerationFailure
// in both modes; API-exhausted (kConfigFetchFailure) only when automatic mode has a next option to
// advance to.
PrepareErrorRouting classifyPrepareError(ConnectError err, bool isAutomaticMode)
{
    if (err == ConnectError::kLocalConfigGenerationFailure ||
        (err == ConnectError::kConfigFetchFailure && isAutomaticMode)) {
        return PrepareErrorRouting::Failover;
    }
    return PrepareErrorRouting::HardStop;
}
