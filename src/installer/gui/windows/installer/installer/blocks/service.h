#pragma once

#include "../iinstall_block.h"

class Service : public IInstallBlock
{
public:
    Service(double weight);
    int executeStep();

private:
    bool installWindscribeService();
};
