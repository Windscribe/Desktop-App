#include "debug_trace.h"

#include <spdlog/details/os.h>

#ifdef DEBUG_TRACE_SPDLOG
#include <atomic>
#else
#include <QString>
#include "categories.h"
#include "utils/extraconfig.h"
#endif

namespace log_utils {

#ifdef DEBUG_TRACE_SPDLOG
namespace {

std::atomic<bool> &helperTraceEnabled()
{
    static std::atomic<bool> enabled{false};
    return enabled;
}

}  // namespace

bool debugTraceEnabled()
{
    return helperTraceEnabled().load(std::memory_order_relaxed);
}

void setDebugTraceEnabled(bool enabled)
{
    helperTraceEnabled().store(enabled, std::memory_order_relaxed);
}
#else
bool debugTraceEnabled()
{
    // Initialized once, thread-safely; later calls are a plain read.
    static const bool enabled = ExtraConfig::instance().getDebugTrace();
    return enabled;
}

void debugTraceLog(const std::string &message)
{
    qCDebug(LOG_DEBUG_TRACE) << QString::fromStdString(message);
}
#endif

std::string debugTraceThreadId()
{
    return std::to_string(spdlog::details::os::thread_id());
}

}  // namespace log_utils
