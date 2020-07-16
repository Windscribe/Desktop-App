#pragma once
#include "basic_command.h"
#include <string>

class InstallTapCommand :
	public BasicCommand
{
public:
	InstallTapCommand(Logger *logger, const wchar_t *tapInstallPath, const wchar_t *infPath, const wchar_t *tapInstallPathForRemove);
	virtual ~InstallTapCommand();

	virtual void execute();

private:
	std::wstring tapInstallPath_;
	std::wstring infPath_;
	std::wstring tapInstallPathForRemove_;
};

