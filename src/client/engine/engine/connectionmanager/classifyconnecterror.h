#pragma once

#include "types/enums.h"

enum class ConnectErrorClassification {
    ErrorAfterDisconnect, // fatal; surfaced via errorDuringConnection once the connector confirms it stopped
    ErrorImmediately, // fatal; stop and surface right away
    Retry, // run the failover/retry logic
    Unknown
};

ConnectErrorClassification classifyConnectError(ConnectError err, bool isAutomaticMode, bool emitAuthError, bool retryOnAttemptFailure);

enum class PrepareErrorRouting {
    Failover, // advance the walk without surfacing an error
    HardStop // retire the connector and surface the error
};

PrepareErrorRouting classifyPrepareError(ConnectError err, bool isAutomaticMode);
