#ifndef INSTALL_SPLITTUNNEL_H
#define INSTALL_SPLITTUNNEL_H

#include <string>
#include <Windows.h>

#include "../iinstall_block.h"


// install split tunnel driver
class InstallSplitTunnel : public IInstallBlock
{
public:
    InstallSplitTunnel(double weight);
    int executeStep();
};

#endif // INSTALL_SPLITTUNNEL_H
