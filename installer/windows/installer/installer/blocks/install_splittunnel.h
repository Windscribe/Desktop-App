#ifndef INSTALL_SPLITTUNNEL_H
#define INSTALL_SPLITTUNNEL_H

#include <string>
#include <Windows.h>

#include "../iinstall_block.h"


// install split tunnel driver
class InstallSplitTunnel : public IInstallBlock
{
public:
    InstallSplitTunnel(const std::wstring& installPath, double weight);
    int executeStep();

private:
    const std::wstring installPath_;
};

#endif // INSTALL_SPLITTUNNEL_H
