#include "command_parser.h"

#include "install_service_command.h"

CommandParser::CommandParser()
{
}

CommandParser::~CommandParser()
{
}

BasicCommand *CommandParser::parse(int argc, wchar_t* argv[])
{
    //	Command for install helper
    // /InstallService log_path service_path
    if (argc == 4 && wcscmp(argv[1], L"/InstallService") == 0)
    {
        Logger *logger = new Logger(argv[2]);
        logger->outCmdLine(argc, argv);
        InstallServiceCommand *cmd = new InstallServiceCommand(logger, argv[3]);
        return cmd;
    }

    return nullptr;
}
