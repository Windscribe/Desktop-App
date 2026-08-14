#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "server.h"
#include "utils.h"
#include "utils/log/spdlog_utils.h"

Server server;

void handler_sigterm(int signum)
{
    UNUSED(signum);
    spdlog::info(WS_PRODUCT_NAME " helper terminated");
    exit(0);
}

int main(int argc, const char *argv[])
{
    // Pin before anything can spawn a child: every command we run as root goes through a shell,
    // and the packages invoke us outside systemd (--reset-mac-addresses from prerm/preun), where
    // the unit's Environment=PATH does not apply and dpkg hands us /usr/local/{sbin,bin} first.
    setenv("PATH", "/usr/sbin:/usr/bin:/sbin:/bin", 1);

    // Initialize logger
    std::string path = WS_LINUX_LOG_DIR;
    //mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
        // Create rotation logger with 2 file with max size 2MB each
        // rotate it on open, the first file is the current log, the 2nd is the previous log
        auto logger = spdlog::rotating_logger_mt("service", logPath, 2 * 1024 * 1024, 1, true);
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

#ifndef NDEBUG
    spdlog::info("Debug build");
#endif

    if (argc > 1 && strcmp(argv[1], "--reset-mac-addresses") == 0) {
        Utils::resetMacAddresses();
        return EXIT_SUCCESS;
    }

    signal(SIGSEGV, handler_sigterm);
    signal(SIGFPE, handler_sigterm);
    signal(SIGABRT, handler_sigterm);
    signal(SIGILL, handler_sigterm);
    signal(SIGINT, handler_sigterm);
    signal(SIGTERM, handler_sigterm);

    server.run();

    spdlog::info(WS_PRODUCT_NAME " helper finished");
    return EXIT_SUCCESS;
}
