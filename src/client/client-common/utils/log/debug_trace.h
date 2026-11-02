#pragma once

// Timing traces for pinpointing runtime stalls. Off unless the ws-debug-trace advanced parameter
// is set. The client checks it once, on first use, so changing the parameter takes effect on the
// next app launch. The helper re-reads it each time a client connects.
//
// Client builds log through Qt (qCDebug). The helper has no Qt and defines DEBUG_TRACE_SPDLOG
// to log through spdlog instead. Both sinks land in the logs the customer already sends.

#include <chrono>
#include <cstdint>
#include <string>

#ifdef DEBUG_TRACE_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace log_utils {

bool debugTraceEnabled();

// The OS thread id (the one debuggers, crash dumps, and system tools show), as a string.
std::string debugTraceThreadId();

#ifdef DEBUG_TRACE_SPDLOG
// Helper only: the service runs as SYSTEM and cannot resolve the customer's app data folder
// until a client connects. Reads the advanced parameters file in folder, resolved from the pipe
// client's token, and replaces the current setting. An empty folder turns tracing off. The
// folder is user-writable, so the caller must be impersonating the client.
void loadDebugTraceSetting(const std::wstring &folder);

inline void debugTraceLog(const std::string &message)
{
    spdlog::debug("{}", message);
}
#else
void debugTraceLog(const std::string &message);
#endif

class DebugTraceScope
{
public:
    explicit DebugTraceScope(const char *name)
        : enabled_(debugTraceEnabled()), name_(name)
    {
        if (!enabled_) {
            return;
        }
        start_ = std::chrono::steady_clock::now();
        debugTraceLog(std::string(name_) + " enter t=" + std::to_string(nowMs())
                      + " tid=" + debugTraceThreadId());
    }

    ~DebugTraceScope()
    {
        if (!enabled_) {
            return;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_).count();
        debugTraceLog(std::string(name_) + " exit " + std::to_string(elapsed) + "ms t="
                      + std::to_string(nowMs()) + " tid=" + debugTraceThreadId());
    }

    DebugTraceScope(const DebugTraceScope &) = delete;
    DebugTraceScope &operator=(const DebugTraceScope &) = delete;

private:
    static std::int64_t nowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    bool enabled_;
    const char *name_;
    std::chrono::steady_clock::time_point start_{};
};

}  // namespace log_utils

#define WS_DEBUG_TRACE_SCOPE(name) log_utils::DebugTraceScope _wsDebugTraceScope(name)
