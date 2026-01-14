#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "utils.h"
#include "utils/crashhandler.h"
#include "utils/log/spdlog_utils.h"
#include "windscribehelper.h"

static bool configureSpdlog()
{
    // Initialize logger
    std::wstring logPath = Utils::getExePath() + L"\\windscribe_service.log";
    auto formatter = log_utils::createJsonFormatter();
    spdlog::set_formatter(std::move(formatter));

    try
    {
        if (log_utils::isOldLogFormat(logPath)) {
            // Use the nothrow version of remove since the removal is not critical and we would
            // like to continue with spdlog creation if the removal does fail.
            std::error_code ec;
            std::filesystem::remove(logPath, ec);
        }
        // Create rotation logger with 2 file with unlimited size
        // rotate it on open, the first file is the current log, the 2nd is the previous log
        auto logger = spdlog::rotating_logger_mt("service", logPath, SIZE_MAX, 1, true);

        // this will trigger flush on every log message
        logger->flush_on(spdlog::level::trace);
        spdlog::set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        // Allow user/us to see this error when the program is running as a service.
        Utils::debugOut("spdlog init failed: %s", ex.what());
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-firewall_off") == 0)) {
        Utils::disableFirewall();
        return 0;
    }

    if (!configureSpdlog()) {
        return 0;
    }

#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName(L"service");
    Debug::CrashHandler::instance().bindToProcess();
#endif

    wsl::WindscribeHelper helper;
    helper.startService();

    return 0;
}
