#pragma once

#include <string>
#include <Windows.h>
#include "../iinstall_block.h"

// install auth helper
class InstallAuthHelper : public IInstallBlock
{
public:
    InstallAuthHelper(double weight);
    ~InstallAuthHelper();
    int executeStep();
};

