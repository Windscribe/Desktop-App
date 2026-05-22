#pragma once

#include "../iinstall_block.h"

// Install tap-windows6 driver (fallback for OpenVPN when DCO is unavailable)
class InstallTapWindows6 : public IInstallBlock
{
public:
    InstallTapWindows6(double weight);
    int executeStep();
};
