#pragma once

#include <string>

#include "../iinstall_block.h"

// install WindscribeService
class Service : public IInstallBlock
{
public:
	Service(double weight);
	int executeStep();

private:
	void installWindscribeService();
	void fixUnquotedServicePath();
	void executeProcess(const std::wstring& process, const std::wstring& cmdLine);
};
