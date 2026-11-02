#include "debug_trace.h"

#include <spdlog/details/os.h>

#ifdef DEBUG_TRACE_SPDLOG
#include <algorithm>
#include <atomic>
#include <cctype>
#include <fstream>
#include <sstream>
#else
#include <QString>
#include "categories.h"
#include "utils/extraconfig.h"
#endif

namespace log_utils {

namespace {

#ifdef DEBUG_TRACE_SPDLOG
// The advanced parameters file is a handful of lines; anything past this is not read.
constexpr std::streamsize kMaxExtraConfigBytes = 64 * 1024;

std::atomic<bool> &helperTraceEnabled()
{
    static std::atomic<bool> enabled{false};
    return enabled;
}

std::string trimmed(const std::string &str)
{
    const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    const auto begin = std::find_if_not(str.begin(), str.end(), isSpace);
    const auto end = std::find_if_not(str.rbegin(), str.rend(), isSpace).base();
    return begin < end ? std::string(begin, end) : std::string();
}

// The helper has no Qt, so it cannot use ExtraConfig. This mirrors ExtraConfig::parseConfigFile:
// a flag is a non-comment line whose key (text before any '=') matches case-insensitively.
bool extraConfigHasDebugTraceFlag(const std::wstring &folder)
{
    if (folder.empty()) {
        return false;
    }
    std::ifstream file(folder + L"\\" WS_EXTRA_CONFIG_NAME, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::string contents(kMaxExtraConfigBytes, '\0');
    file.read(contents.data(), kMaxExtraConfigBytes);
    contents.resize(static_cast<size_t>(file.gcount()));

    std::istringstream stream(contents);
    std::string line;
    while (std::getline(stream, line)) {
        std::string key = trimmed(line.substr(0, line.find('=')));
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (key == "ws-debug-trace") {
            return true;
        }
    }
    return false;
}
#endif

}  // namespace

#ifdef DEBUG_TRACE_SPDLOG
bool debugTraceEnabled()
{
    return helperTraceEnabled().load(std::memory_order_relaxed);
}

void loadDebugTraceSetting(const std::wstring &folder)
{
    helperTraceEnabled().store(extraConfigHasDebugTraceFlag(folder), std::memory_order_relaxed);
}
#else
bool debugTraceEnabled()
{
#ifdef _WIN32
    // Initialized once, thread-safely; later calls are a plain read.
    static const bool enabled = ExtraConfig::instance().getDebugTrace();
    return enabled;
#else
    return false;
#endif
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
