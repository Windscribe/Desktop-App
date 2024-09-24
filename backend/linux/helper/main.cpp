#include <syslog.h>
#include <signal.h>
#include "server.h"
#include "logger.h"
#include "utils.h"

Server server;

void handler_sigterm(int signum)
{
    UNUSED(signum);
    Logger::instance().out("Windscribe helper terminated");
    exit(0);
}

int main(int argc, const char *argv[])
{
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

    Logger::instance().checkLogSize();

    server.run();

    Logger::instance().out("Windscribe helper finished");
    return EXIT_SUCCESS;
}
