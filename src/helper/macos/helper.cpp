#include <syslog.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <sys/stat.h>
#include <stdio.h>
#include "utils/log/spdlog_utils.h"
#include "server.h"

Server server;

void handler_sigterm(int signum)
{
    spdlog::error("Windscribe helper terminated");
    exit(0);
}

int main(int argc, const char *argv[])
{
    // Initialize logger
    std::string path = "/Library/Logs/com.windscribe.helper.macos";
    std::string logPath = path + "/helper.log";

    // remove old unused log file from prev versions of the helper
    std::string oldLogPath = path + "/helper_log.txt";
    remove(oldLogPath.c_str());

    auto formatter = log_utils::createJsonFormatter();
    spdlog::set_formatter(std::move(formatter));

    try
    {
        if (log_utils::isOldLogFormat(logPath)) {
            remove(logPath.c_str());
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
        printf("spdlog init failed: %s\n", ex.what());
        return 0;
    }

    spdlog::info("=== Started ===");

    signal(SIGSEGV, handler_sigterm);
    signal(SIGFPE, handler_sigterm);
    signal(SIGABRT, handler_sigterm);
    signal(SIGILL, handler_sigterm);
    signal(SIGINT, handler_sigterm);
    signal(SIGTERM, handler_sigterm);

    server.run();

    spdlog::info("Windscribe helper finished");
    return EXIT_SUCCESS;
}
