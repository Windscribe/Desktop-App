#pragma once
#include "basic_command.h"
#include <string>

class UninstallTapCommand :
	public BasicCommand
{
public:
	UninstallTapCommand(Logger *logger, const wchar_t *tapInstallPath);
	virtual ~UninstallTapCommand();

	virtual void execute();

private:
	std::wstring tapInstallPath_;
};

