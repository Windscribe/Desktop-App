#include "command_parser.h"
#include "install_service_command.h"
#include "install_tap_command.h"
#include "uninstall_tap_command.h"
#include "logger.h"
#include <windows.h>

CommandParser::CommandParser()
{
}


CommandParser::~CommandParser()
{
}

BasicCommand *CommandParser::parse(int argc, wchar_t* argv[])
{
	//	Command for install Windscribe helper
	// /InstallService log_path service_path subinacl_path
	if (argc == 5 && wcscmp(argv[1], L"/InstallService") == 0)
	{
		Logger *logger = new Logger(argv[2]);
		logger->outCmdLine(argc, argv);
		InstallServiceCommand *cmd = new InstallServiceCommand(logger, argv[3], argv[4]);
		return cmd;
	}
	//	Command for install TAP-adapter
	// /InstallTap log_path tap_install_path inf_path
	else if (argc == 5 && wcscmp(argv[1], L"/InstallTap") == 0)
	{
		Logger *logger = new Logger(argv[2]);
		logger->outCmdLine(argc, argv);
		InstallTapCommand *cmd = new InstallTapCommand(logger, argv[3], argv[4], L"");
		return cmd;
	}
	//	Command for install TAP-adapter with remove previous
	// /InstallTapWithRemovePrevious log_path tap_install_path inf_path <tap_install_path_for_remove_cmd>
	else if (argc == 6 && wcscmp(argv[1], L"/InstallTapWithRemovePrevious") == 0)
	{
		Logger *logger = new Logger(argv[2]);
		logger->outCmdLine(argc, argv);
		InstallTapCommand *cmd = new InstallTapCommand(logger, argv[3], argv[4], argv[5]);
		return cmd;
	}
	//	Command for uninstall TAP-adapter
	// /UninstallTap log_path tap_install_path
	else if (argc == 4 && wcscmp(argv[1], L"/UninstallTap") == 0)
	{
		Logger *logger = new Logger(argv[2]);
		logger->outCmdLine(argc, argv);
		UninstallTapCommand *cmd = new UninstallTapCommand(logger, argv[3]);
		return cmd;
	}
	else
	{
		return 0;
	}
}
