#pragma once

#include "../iinstall_block.h"

// Install OpenVPN DCO kernel driver
class InstallOpenVPNDCO : public IInstallBlock
{
public:
    InstallOpenVPNDCO(double weight);
    int executeStep();
};
