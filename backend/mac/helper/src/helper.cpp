#include <syslog.h>
#include "server.h"
#include "logger.h"

Server server;

void handler_sigterm(int signum)
{
    LOG("Windscribe helper terminated");
    exit(0);
}

int main(int argc, const char *argv[])
{
    signal(SIGSEGV, handler_sigterm);
    signal(SIGFPE, handler_sigterm);
    signal(SIGABRT, handler_sigterm);
    signal(SIGILL, handler_sigterm);
    signal(SIGINT, handler_sigterm);
    signal(SIGTERM, handler_sigterm);
    
    server.run();
    
    LOG("Windscribe helper finished");
    return EXIT_SUCCESS;
}

