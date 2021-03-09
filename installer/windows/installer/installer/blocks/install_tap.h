#ifndef INSTALL_TAP_H
#define INSTALL_TAP_H
#include <string>
#include <Windows.h>
#include "../iinstall_block.h"


// install tap drivers
class InstallTap : public IInstallBlock
{
public:
	InstallTap(const std::wstring &installPath, double weight);
	int executeStep();

private:
	std::wstring installPath_;
};

#endif // INSTALL_TAP_H
