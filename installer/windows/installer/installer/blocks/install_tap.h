#ifndef INSTALL_TAP_H
#define INSTALL_TAP_H

#include <string>
#include <Windows.h>

#include "../iinstall_block.h"


// install tap drivers
class InstallTap : public IInstallBlock
{
public:
	InstallTap(double weight);
	int executeStep();
};

#endif // INSTALL_TAP_H
