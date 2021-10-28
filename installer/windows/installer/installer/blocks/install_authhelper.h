#pragma once

#include <string>
#include <Windows.h>
#include "../iinstall_block.h"

// install auth helper
class InstallAuthHelper : public IInstallBlock
{
public:
	InstallAuthHelper(const std::wstring &installPath, double weight);
	~InstallAuthHelper();
	int executeStep();

private:
	std::wstring installPath_;
};

