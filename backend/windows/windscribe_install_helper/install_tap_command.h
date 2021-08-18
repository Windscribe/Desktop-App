#pragma once
#include "basic_command.h"
#include <string>

class InstallTapCommand :
	public BasicCommand
{
public:
	InstallTapCommand(Logger *logger, const wchar_t *tapInstallPath, const wchar_t *infPath, const wchar_t *tapInstallPathForRemove);
	~InstallTapCommand() override;

	void execute() override;

private:
	std::wstring tapInstallPath_;
	std::wstring infPath_;
	std::wstring tapInstallPathForRemove_;
};

