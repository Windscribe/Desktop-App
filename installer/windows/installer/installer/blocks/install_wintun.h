#ifndef INSTALL_WINTUN_H
#define INSTALL_WINTUN_H
#include <string>
#include <Windows.h>
#include "../iinstall_block.h"


// install wintun driver
class InstallWinTun : public IInstallBlock
{
public:
	InstallWinTun(const std::wstring &installPath, double weight);
	int executeStep();

private:
	std::wstring installPath_;
};

#endif // INSTALL_WINTUN_H
