#pragma once

namespace server_api {

enum class SessionErrorCode { kSuccess, kSessionInvalid, kBadUsername, kAccountDisabled, kMissingCode2FA, kBadCode2FA, kUnknownError };

}
