#ifndef SERVICE_H
#define SERVICE_H

#include <string>
#include <Windows.h>

#include "../iinstall_block.h"


// install WindscribeService
class Service : public IInstallBlock
{
public:
	Service(double weight);
	int executeStep();

private:
	unsigned long installWindscribeService();
	void fixUnquotedServicePath();
};

#endif // SERVICE_H
