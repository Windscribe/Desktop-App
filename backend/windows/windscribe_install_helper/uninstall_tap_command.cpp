#include "uninstall_tap_command.h"


UninstallTapCommand::UninstallTapCommand(Logger *logger, const wchar_t *tapInstallPath) :
	BasicCommand(logger), tapInstallPath_(tapInstallPath)
{
	tapInstallPath_.insert(0, 1, L'"');
	tapInstallPath_.insert(tapInstallPath_.length(), 1, L'"');
	tapInstallPath_.append(L" remove tapwindscribe0901");
}

UninstallTapCommand::~UninstallTapCommand()
{
}

void UninstallTapCommand::execute()
{
	logger_->outStr(tapInstallPath_.c_str());
	logger_->outStr("\n");
	std::string answer;
	if (executeBlockingCommand(tapInstallPath_.c_str(), answer))
	{
		logger_->outStr(answer.c_str());
	}
	else
	{
		logger_->outStr(L"Failed to execute command with executeBlockingCommand\n");
	}
}

