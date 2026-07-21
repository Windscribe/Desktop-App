#pragma once

#include <Windows.h>

#include <string>

namespace wsl {

// Incremental, crash-surviving log file in %ProgramData%\Windscribe.
// This is the single source of truth for installer/uninstaller logs:
// - it is written line-by-line with no buffering, so it survives a crash of the process;
// - lives outside the install folder, so it survives removal of the previous install
//   during an upgrade;
// - is rotated (file.log -> file.log.1) on open, so a retry does not destroy the
//   evidence of the first failure.
//
// The directory is created with a DACL granting full access to Administrators/SYSTEM
// and read-only access to Users, so a regular user can hand the log to support but
// cannot tamper with it while an elevated installer is running.  All failures are
// non-fatal: if the log file cannot be opened safely, logging to it is silently
// disabled and diagnostics are emitted via OutputDebugString.
class PersistentLog
{
public:
    PersistentLog() = default;
    ~PersistentLog();

    PersistentLog(const PersistentLog &) = delete;
    PersistentLog &operator=(const PersistentLog &) = delete;

    enum class OpenMode {
        Rotate,  // rotate fileName -> fileName.1 and start a new file
        Append,  // continue an existing file (e.g. the uninstaller's second phase
                 // continues the log started by the first phase)
    };

    // The directory holding the logs: %ProgramData%\Windscribe.
    // Returns an empty string if the ProgramData folder cannot be resolved.
    static std::wstring defaultDirectory();

    // Full path of the log file with the given name: %ProgramData%\Windscribe\<fileName>.
    // Returns an empty string if the ProgramData folder cannot be resolved.
    static std::wstring defaultPath(const std::wstring &fileName);

    // Creates/validates %ProgramData%\Windscribe and opens the log file.
    // Returns false if file logging is unavailable.
    bool open(const std::wstring &fileName, OpenMode mode = OpenMode::Rotate);
    void close();

    bool isOpen() const { return handle_ != INVALID_HANDLE_VALUE; }
    std::wstring path() const { return path_; }

    // Appends the UTF-8 encoded entry to the file.  The entry is expected to be
    // newline-terminated (spdlog formatters append '\n').  Write-through, no buffering.
    void write(const std::string &utf8);

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::wstring path_;
};

}
