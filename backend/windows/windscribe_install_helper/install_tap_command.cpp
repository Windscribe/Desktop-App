#include "install_tap_command.h"


InstallTapCommand::InstallTapCommand(Logger *logger, const wchar_t *tapInstallPath, const wchar_t *infPath,
									 const wchar_t *tapInstallPathForRemove) :
		BasicCommand(logger), tapInstallPath_(tapInstallPath), infPath_(infPath), tapInstallPathForRemove_(tapInstallPathForRemove)
{
	if (!tapInstallPathForRemove_.empty())
	{
		tapInstallPathForRemove_.insert(0, 1, L'"');
		tapInstallPathForRemove_.insert(tapInstallPathForRemove_.length(), 1, L'"');
		tapInstallPathForRemove_.append(L" remove tapwindscribe0901");
	}

}

InstallTapCommand::~InstallTapCommand()
{
}

void InstallTapCommand::execute()
{
	std::string answer;
	if (!tapInstallPathForRemove_.empty())
	{
		logger_->outStr(tapInstallPathForRemove_.c_str());
		logger_->outStr("\n");
		if (executeBlockingCommand(tapInstallPathForRemove_.c_str(), answer))
		{
			logger_->outStr(answer.c_str());
		}
		else
		{
			logger_->outStr(L"Failed to execute command with executeBlockingCommand\n");
		}
	}

	std::wstring cmd = L"\"" + tapInstallPath_ + L"\" install ";
	cmd += L"\"" + infPath_ + L"\" tapwindscribe0901";
	logger_->outStr(cmd.c_str());
	logger_->outStr("\n");
	if (executeBlockingCommand(cmd.c_str(), answer))
	{
		logger_->outStr(answer.c_str());
	}
	else
	{
		logger_->outStr(L"Failed to execute command with executeBlockingCommand\n");
	}
}
