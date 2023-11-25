#pragma once

#include <string>
#include <Windows.h>

#include "../iinstall_block.h"

// install split tunnel driver
class InstallSplitTunnel : public IInstallBlock
{
public:
    InstallSplitTunnel(double weight);
    int executeStep();
    bool serviceInstalled() const;
};
