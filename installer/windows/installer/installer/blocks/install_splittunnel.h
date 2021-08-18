#ifndef INSTALL_SPLITTUNNEL_H
#define INSTALL_SPLITTUNNEL_H
#include <string>
#include <Windows.h>
#include "../iinstall_block.h"


// install split tunnel driver
class InstallSplitTunnel : public IInstallBlock
{
public:
	InstallSplitTunnel(const std::wstring &installPath, double weight, HWND hwnd);
	int executeStep();

private:
	std::wstring installPath_;
	HWND hwnd_;
};

#endif // INSTALL_SPLITTUNNEL_H
