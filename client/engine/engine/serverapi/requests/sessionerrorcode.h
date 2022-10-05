#pragma once

enum class SessionErrorCode { kSuccess, kSessionInvalid, kBadUsername, kAccountDisabled, kMissingCode2FA, kBadCode2FA, kUnknownError };