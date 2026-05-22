#pragma once

#include <Windows.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace wsl {

std::optional<std::wstring> generateRandomSuffix();

// Atomically creates a directory whose DACL grants full access only to
// BUILTIN\Administrators and NT AUTHORITY\SYSTEM, with inheritance from the
// parent blocked.  Returns 0 on success, or a Win32 error code on failure;
// the caller can branch on ERROR_ALREADY_EXISTS to distinguish a collision
// from other failures.
DWORD createAdminOnlyDirectory(const std::filesystem::path &path,
                               const std::function<void(const std::wstring&)> &logFunc);

}
