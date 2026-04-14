#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

#import <grp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#import "TransparentProxyProvider.h"

int main(int argc, char *argv[]) {
    @autoreleasepool {
        // Initialize logger
        std::string path = "/Library/Logs/" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID;
        std::string logPath = path + "/splittunnelextension.log";

        try {
            // Create rotation logger with 2 files with max size 2MB each
            // rotate it on open, the first file is the current log, the 2nd is the previous log
            auto logger = spdlog::rotating_logger_mt("splittunnelextension", logPath, 2 * 1024 * 1024, 1, true);
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
        spdlog::info("Starting " WS_PRODUCT_NAME " Split Tunnel Provider");

        // Get app group info and set gid
        struct group *grp = getgrnam(WS_PRODUCT_NAME_LOWER);
        if (grp == NULL) {
            spdlog::error("Failed to get app group info");
            return 1;
        }

        if (setgid(grp->gr_gid) != 0) {
            spdlog::error("Failed to set gid to app group");
            return 1;
        }

        @autoreleasepool {
            TransparentProxyProvider *provider = [[TransparentProxyProvider alloc] init];
            (void)provider; // Provider is registered with the system during init

            // Start handling proxy requests
            [NEProvider startSystemExtensionMode];
        }

        // Run the extension indefinitely
        [[NSRunLoop mainRunLoop] run];
    }
    return 0;
}
