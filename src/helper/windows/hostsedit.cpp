#include "hostsedit.h"

#include <restartmanager.h>
#include <shlwapi.h>

#include <cctype>
#include <chrono>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include "utils.h"
#include "utils/win32handle.h"
#include "utils/wsscopeguard.h"

namespace {

// hosts is a byte file. The CRT used to read and write it in the "C" locale, which passes each byte
// through unchanged, so a comment stored as Windows-1251 or UTF-8 survives. Keep that behavior.
constexpr DWORD kReadChunkSize = 64 * 1024;

// Readers (DNS client, AV scanners) may open the file while we hold it, but no other writer may. An
// open fails with a sharing violation while another process has the file open for writing, which the
// open retry waits out. Once we hold it, nobody else can change the file until our handle closes.
constexpr DWORD kShareMode = FILE_SHARE_READ;

// A hosts file of several thousand lines may be held locked by the other processes (e.g. DNS client/AV scanner)
// for a fraction of a second, long enough to fail a single open or replace. Wait that out. The helper command
// thread blocks for the whole wait; the pipe read on the engine side has no timeout.
constexpr auto kHostsRetryTimeout = std::chrono::seconds(5);
constexpr auto kHostsRetryInterval = std::chrono::milliseconds(50);
using HostsRetryDeadline = std::chrono::steady_clock::time_point;

bool isRetryableHostsError(DWORD error, bool replacing)
{
    // Replacing a destination opened without FILE_SHARE_DELETE reports ERROR_ACCESS_DENIED as well
    // as ERROR_SHARING_VIOLATION. An open that returns access denied is a permission failure.
    return error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION ||
           (replacing && error == ERROR_ACCESS_DENIED);
}

bool isReadOnlyFile(const std::wstring &path)
{
    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0;
}

// Returns true when the caller should try again. A readonly file, a non-retryable error, and a
// deadline that has already passed are all immediate failures. The sleep happens only on the way
// to another attempt, so a conflict that clears on the first retry still finishes inside the budget.
bool waitForHostsRetry(DWORD error, const std::wstring &path, const HostsRetryDeadline &deadline, bool replacing)
{
    if (!isRetryableHostsError(error, replacing) || isReadOnlyFile(path) || std::chrono::steady_clock::now() >= deadline) {
        return false;
    }
    std::this_thread::sleep_for(kHostsRetryInterval);
    return true;
}

std::string wideAsciiToBytes(const std::wstring &text)
{
    std::string bytes;
    bytes.reserve(text.size());
    for (const wchar_t ch : text) {
        bytes.push_back(static_cast<char>(ch));
    }
    return bytes;
}

// A hosts line ends at \n, \r\n, or a bare \r. The view includes the ending bytes so a rewrite can
// copy them unchanged. The final line of a file that has no ending is returned as-is.
bool nextLine(std::string_view &remaining, std::string_view &line)
{
    if (remaining.empty()) {
        return false;
    }

    const size_t end = remaining.find_first_of("\r\n");
    if (end == std::string_view::npos) {
        line = remaining;
        remaining = {};
        return true;
    }

    size_t length = end + 1;
    if (remaining[end] == '\r' && length < remaining.size() && remaining[length] == '\n') {
        ++length;
    }
    line = remaining.substr(0, length);
    remaining.remove_prefix(length);
    return true;
}

bool lineEndsWithNewline(std::string_view line)
{
    return !line.empty() && (line.back() == '\n' || line.back() == '\r');
}

std::wstring win32ErrorMessage(DWORD error)
{
    wchar_t *message = nullptr;
    const DWORD length = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                              FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                          nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                          reinterpret_cast<wchar_t *>(&message), 0, nullptr);
    auto freeMessage = wsl::wsScopeGuard([&] {
        if (message) {
            ::LocalFree(message);
        }
    });

    if (length == 0 || message == nullptr) {
        return L"Unknown error code " + std::to_wstring(error);
    }
    return std::wstring(message, length);
}

// Restart Manager names the processes that have the file open. It is only meaningful for a sharing
// violation: the other failures are permissions, a missing file, or a bad path, and no process list
// explains them. The process may release the file between the failed call and this query.
void logFileLockers(const std::wstring &path)
{
    DWORD session = 0;
    wchar_t sessionKey[CCH_RM_SESSION_KEY + 1] = {};
    DWORD result = ::RmStartSession(&session, 0, sessionKey);
    if (result != ERROR_SUCCESS) {
        spdlog::error(L"HostsEdit: RmStartSession failed ({}) for {}", result, path);
        return;
    }
    auto endSession = wsl::wsScopeGuard([&] { ::RmEndSession(session); });

    LPCWSTR resources[] = { path.c_str() };
    result = ::RmRegisterResources(session, 1, resources, 0, nullptr, 0, nullptr);
    if (result != ERROR_SUCCESS) {
        spdlog::error(L"HostsEdit: RmRegisterResources failed ({}) for {}", result, path);
        return;
    }

    // The list can grow between the size query and the read, so repeat a few times.
    constexpr int kMaxAttempts = 3;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        UINT needed = 0;
        UINT count = 0;
        DWORD rebootReasons = 0;
        result = ::RmGetList(session, &needed, &count, nullptr, &rebootReasons);
        if (result == ERROR_SUCCESS) {
            spdlog::error(L"HostsEdit: no process reported holding {}", path);
            return;
        }
        if (result != ERROR_MORE_DATA) {
            spdlog::error(L"HostsEdit: RmGetList failed ({}) for {}", result, path);
            return;
        }

        std::vector<RM_PROCESS_INFO> processes(needed);
        count = needed;
        result = ::RmGetList(session, &needed, &count, processes.data(), &rebootReasons);
        if (result == ERROR_MORE_DATA) {
            continue;
        }
        if (result != ERROR_SUCCESS) {
            spdlog::error(L"HostsEdit: RmGetList failed ({}) for {}", result, path);
            return;
        }

        for (UINT i = 0; i < count; ++i) {
            const RM_PROCESS_INFO &info = processes[i];
            const wchar_t *appName = info.strAppName[0] != L'\0' ? info.strAppName : L"(unnamed)";
            if (info.strServiceShortName[0] != L'\0') {
                spdlog::error(L"HostsEdit: {} is held by {} service {} (pid {})", path, appName,
                              info.strServiceShortName, info.Process.dwProcessId);
            } else {
                spdlog::error(L"HostsEdit: {} is held by {} (pid {})", path, appName, info.Process.dwProcessId);
            }
        }
        return;
    }

    spdlog::error(L"HostsEdit: process list for {} changed while it was being read", path);
}

}  // namespace

HostsEdit::HostsEdit() : addedByMarker_("added by " WS_PRODUCT_NAME ", do not modify.")
{
    systemDir_ = Utils::getSystemDir();
}

HostsEdit::~HostsEdit()
{
}

bool HostsEdit::addHosts(const std::wstring &ip, const std::wstring &hostname)
{
    const auto hostsPath = getHostsPath();
    // One clear, up front. A file that is readonly again on a later attempt fails immediately.
    bool wasReadOnly = false;
    if (!ensureHostsFileWritable(hostsPath, true, wasReadOnly)) {
        return false;
    }
    // Put back the read-only attribute the user or a policy set, whether or not the edit succeeded.
    auto restoreAttribute = wsl::wsScopeGuard([&] {
        if (wasReadOnly) {
            restoreReadOnly(hostsPath);
        }
    });

    const auto deadline = std::chrono::steady_clock::now() + kHostsRetryTimeout;
    // IP and hostname are validated as ASCII before this call, and the marker is ASCII.
    const std::string entry = wideAsciiToBytes(ip) + " " + wideAsciiToBytes(hostname);

    for (;;) {
        const wsl::Win32Handle file(openFile(hostsPath, GENERIC_READ | GENERIC_WRITE, OPEN_ALWAYS));
        if (!file.isValid()) {
            const DWORD error = ::GetLastError();
            if (waitForHostsRetry(error, hostsPath, deadline, false)) {
                continue;
            }
            logWin32Error("CreateFile failed", hostsPath, error);
            return false;
        }

        // No other writer can open the file while we hold it, so only the open is retried. A read or
        // write failure from here on is not a transient sharing conflict.
        std::string bytes;
        if (!readFileBytes(file.getHandle(), bytes)) {
            logWin32Error("ReadFile of hosts file failed", hostsPath, ::GetLastError());
            return false;
        }

        std::vector<std::string_view> lines;
        std::string_view remaining(bytes);
        std::string_view line;
        while (nextLine(remaining, line)) {
            lines.push_back(line);
        }

        if (stringInLines(lines, entry)) {
            return true;
        }

        // The CRT text mode appended at the end and translated \n to \r\n. A file that already ends
        // with a line ending needs no separator; an empty file needs none either.
        std::string addition;
        if (!lines.empty() && !lineEndsWithNewline(lines.back())) {
            addition += "\r\n";
        }
        addition += entry;
        addition += "   #";
        addition += addedByMarker_;
        addition += "\r\n";

        // The whole file was just read, so the file pointer is already at its end.
        LARGE_INTEGER originalSize = {};
        if (::SetFilePointerEx(file.getHandle(), {}, &originalSize, FILE_CURRENT) == FALSE) {
            logWin32Error("SetFilePointerEx on hosts file failed", hostsPath, ::GetLastError());
            return false;
        }
        if (addition.size() > (std::numeric_limits<DWORD>::max)()) {
            spdlog::error("HostsEdit: hosts addition is larger than a DWORD byte count");
            return false;
        }
        if (!writeFileBytes(file.getHandle(), addition.data(), static_cast<DWORD>(addition.size()))) {
            const DWORD writeError = ::GetLastError();
            logWin32Error("WriteFile of hosts file failed", hostsPath, writeError);
            // A partial write would leave a fragment without the marker, which removeHosts never
            // removes. Cut the file back to its original length.
            if (::SetFilePointerEx(file.getHandle(), originalSize, nullptr, FILE_BEGIN) == FALSE ||
                ::SetEndOfFile(file.getHandle()) == FALSE) {
                logWin32Error("Truncating hosts file after a failed write failed", hostsPath, ::GetLastError());
            }
            return false;
        }
        return true;
    }
}

bool HostsEdit::removeHosts()
{
    const auto hostsPath = getHostsPath();
    const auto tempHostsPath = getTempHostsPath();
    bool wasReadOnly = false;
    if (!ensureHostsFileWritable(hostsPath, false, wasReadOnly)) {
        return false;
    }
    // The replacement is created with FILE_ATTRIBUTE_NORMAL, so a successful move also drops the
    // attribute. Set it on whichever file ends up at hostsPath.
    auto restoreAttribute = wsl::wsScopeGuard([&] {
        if (wasReadOnly) {
            restoreReadOnly(hostsPath);
        }
    });

    const auto deadline = std::chrono::steady_clock::now() + kHostsRetryTimeout;
    std::string prepared;
    bool tempPrepared = false;

    for (;;) {
        // Re-read so an edit made by another program during the wait is kept. Rewrite hosts.tmp
        // only when that result changed: rewriting it on every tick retriggers directory scanners
        // and can hold the destination locked until the deadline expires.
        std::string rewritten;
        {
            const wsl::Win32Handle hostsFile(openFile(hostsPath, GENERIC_READ, OPEN_EXISTING));
            if (!hostsFile.isValid()) {
                const DWORD error = ::GetLastError();
                if (waitForHostsRetry(error, hostsPath, deadline, false)) {
                    continue;
                }
                logWin32Error("CreateFile failed", hostsPath, error);
                return false;
            }

            std::string bytes;
            if (!readFileBytes(hostsFile.getHandle(), bytes)) {
                logWin32Error("ReadFile of hosts file failed", hostsPath, ::GetLastError());
                return false;
            }

            std::vector<std::string_view> kept;
            std::string_view remaining(bytes);
            std::string_view line;
            while (nextLine(remaining, line)) {
                if (line.find(addedByMarker_) == std::string_view::npos) {
                    kept.push_back(line);
                }
            }

            rewritten.reserve(bytes.size());
            // Kept lines are copied with their endings. addHosts adds a separator only when the file
            // did not end in one, so a file we did not otherwise change is written back byte-for-byte.
            for (const std::string_view keptLine : kept) {
                rewritten.append(keptLine);
            }
        }

        if (!tempPrepared || rewritten != prepared) {
            if (rewritten.size() > (std::numeric_limits<DWORD>::max)()) {
                spdlog::error("HostsEdit: rewritten hosts file is larger than a DWORD byte count");
                return false;
            }

            const wsl::Win32Handle tempFile(openFile(tempHostsPath, GENERIC_WRITE, CREATE_ALWAYS));
            if (!tempFile.isValid()) {
                const DWORD error = ::GetLastError();
                if (waitForHostsRetry(error, tempHostsPath, deadline, false)) {
                    continue;
                }
                logWin32Error("CreateFile failed", tempHostsPath, error);
                return false;
            }
            if (!writeFileBytes(tempFile.getHandle(), rewritten.data(), static_cast<DWORD>(rewritten.size()))) {
                logWin32Error("WriteFile of hosts file failed", tempHostsPath, ::GetLastError());
                return false;
            }
            prepared = std::move(rewritten);
            tempPrepared = true;
        }

        // Both handles are closed before the move. MoveFileEx cannot replace a file this process holds.
        if (::MoveFileExW(tempHostsPath.c_str(), hostsPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE) {
            return true;
        }
        const DWORD error = ::GetLastError();
        if (waitForHostsRetry(error, hostsPath, deadline, true)) {
            continue;
        }
        logWin32Error("MoveFileEx of temp hosts file over hosts file failed", hostsPath, error, true);
        return false;
    }
}

bool HostsEdit::ensureHostsFileWritable(const std::wstring &path, bool allowMissing, bool &wasReadOnly) const
{
    wasReadOnly = false;
    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = ::GetLastError();
        if (allowMissing && error == ERROR_FILE_NOT_FOUND) {
            return true;
        }
        logWin32Error("GetFileAttributes of hosts file failed", path, error);
        return false;
    }

    // CreateFileW reports a directory as access denied, so say what the path is before opening it.
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        spdlog::error(L"Hosts file path is a directory: {}", path);
        return false;
    }

    if ((attributes & FILE_ATTRIBUTE_READONLY) == 0) {
        return true;
    }

    spdlog::info(L"Hosts file is read-only; clearing the read-only attribute: {}", path);
    if (::SetFileAttributesW(path.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY) == FALSE) {
        logWin32Error("SetFileAttributes of hosts file failed", path, ::GetLastError());
        return false;
    }
    wasReadOnly = true;
    return true;
}

void HostsEdit::restoreReadOnly(const std::wstring &path) const
{
    // The edit has already been made, so a failure here is logged and does not fail the command.
    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        logWin32Error("GetFileAttributes of hosts file in restoreReadOnly failed", path, ::GetLastError());
        return;
    }
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0) {
        return;
    }
    if (::SetFileAttributesW(path.c_str(), attributes | FILE_ATTRIBUTE_READONLY) == FALSE) {
        logWin32Error("SetFileAttributes to restore read-only on hosts file failed", path, ::GetLastError());
        return;
    }
}

HANDLE HostsEdit::openFile(const std::wstring &path, DWORD desiredAccess, DWORD creationDisposition) const
{
    // The caller logs. Logging here would emit one line per 50 ms retry.
    return ::CreateFileW(path.c_str(), desiredAccess, kShareMode, nullptr, creationDisposition,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
}

bool HostsEdit::readFileBytes(HANDLE file, std::string &bytes) const
{
    for (;;) {
        const size_t offset = bytes.size();
        if (offset > static_cast<size_t>((std::numeric_limits<DWORD>::max)()) - kReadChunkSize) {
            spdlog::error("HostsEdit: hosts file is larger than a DWORD byte count");
            ::SetLastError(ERROR_FILE_TOO_LARGE);
            return false;
        }
        bytes.resize(offset + kReadChunkSize);
        DWORD read = 0;
        if (::ReadFile(file, bytes.data() + offset, kReadChunkSize, &read, nullptr) == FALSE) {
            return false;
        }
        bytes.resize(offset + read);
        if (read < kReadChunkSize) {
            return true;
        }
    }
}

bool HostsEdit::writeFileBytes(HANDLE file, const void *data, DWORD size) const
{
    const auto *cursor = static_cast<const char *>(data);
    DWORD remaining = size;
    while (remaining > 0) {
        DWORD written = 0;
        if (::WriteFile(file, cursor, remaining, &written, nullptr) == FALSE) {
            return false;
        }
        if (written == 0) {
            ::SetLastError(ERROR_WRITE_FAULT);
            return false;
        }
        cursor += written;
        remaining -= written;
    }
    return true;
}

void HostsEdit::logWin32Error(const char *what, const std::wstring &path, DWORD error, bool logLockers) const
{
    spdlog::error(L"HostsEdit: {} ({}) for {} ({})", std::wstring(what, what + std::strlen(what)), error, path,
                  win32ErrorMessage(error));
    // A failed replace reports access denied when the holder did not share delete. Ask who holds it.
    // An access-denied open is permissions or a directory, and no process list explains it.
    if (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION ||
        (logLockers && error == ERROR_ACCESS_DENIED)) {
        logFileLockers(path);
    }
}

std::wstring HostsEdit::getHostsPath() const
{
    wchar_t path[MAX_PATH];
    PathCombineW(path, systemDir_.c_str(), L"drivers\\etc\\hosts");
    return path;
}

std::wstring HostsEdit::getTempHostsPath() const
{
    wchar_t path[MAX_PATH];
    PathCombineW(path, systemDir_.c_str(), L"drivers\\etc\\hosts.tmp");
    return path;
}

bool HostsEdit::stringInLines(const std::vector<std::string_view> &lines, const std::string &str) const
{
    if (str.empty()) {
        return false;
    }

    for (const std::string_view line : lines) {
        // Match the entry exactly, so a shorter hostname that is a prefix of another does not match.
        for (size_t pos = line.find(str); pos != std::string_view::npos; pos = line.find(str, pos + 1)) {
            const bool boundaryBefore = (pos == 0) || std::isspace(static_cast<unsigned char>(line[pos - 1]));
            const size_t after = pos + str.size();
            const bool boundaryAfter = (after == line.size()) || std::isspace(static_cast<unsigned char>(line[after])) ||
                                       line[after] == '#';
            if (boundaryBefore && boundaryAfter) {
                return true;
            }
        }
    }
    return false;
}
