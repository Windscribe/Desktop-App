#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

#import <grp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

int main(int argc, char *argv[]) {
    @autoreleasepool {
        // Initialize logger
        std::string path = "/Library/Logs/com.windscribe.splittunnelextension";
        std::string logPath = path + "/splittunnelextension.log";

        try {
            // Create rotation logger with 2 files with unlimited size
            // rotate it on open, the first file is the current log, the 2nd is the previous log
            auto logger = spdlog::rotating_logger_mt("splittunnelextension", logPath, SIZE_MAX, 1, true);
            // this will trigger flush on every log message
            logger->flush_on(spdlog::level::err);
            spdlog::set_level(spdlog::level::err);
            spdlog::set_default_logger(logger);
        }
        catch (const spdlog::spdlog_ex &ex) {
            printf("spdlog init failed: %s\n", ex.what());
            return 1;
        }

        // Create and start the provider
        spdlog::info("Starting Windscribe Split Tunnel Provider");

        // Get windscribe group info and set gid
        struct group *grp = getgrnam("windscribe");
        if (grp == NULL) {
            spdlog::error("Failed to get windscribe group info");
            return 1;
        }

        if (setgid(grp->gr_gid) != 0) {
            spdlog::error("Failed to set gid to windscribe group");
            return 1;
        }

        // Start handling proxy requests
        [NEProvider startSystemExtensionMode];

        // Run the extension indefinitely
        [[NSRunLoop mainRunLoop] run];
    }
    return 0;
}
