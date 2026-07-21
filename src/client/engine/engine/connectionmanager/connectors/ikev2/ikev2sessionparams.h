#pragma once

#include "api_responses/servercredentials.h"

// Session-scoped IKEv2 data, fixed at clickConnect and handed to the connector by the factory.
struct Ikev2SessionParams
{
    api_responses::ServerCredentials serverCredentials;
};
