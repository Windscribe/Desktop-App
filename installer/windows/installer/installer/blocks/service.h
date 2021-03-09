#ifndef SERVICE_H
#define SERVICE_H
#include <string>
#include <Windows.h>
#include "../iinstall_block.h"


//#include "../../Utils/services.h"

// install WindscribeService
class Service : public IInstallBlock
{
public:
	Service(const std::wstring &installPath, double weight);
	int executeStep();

private:
	std::wstring installPath_;

	unsigned long installWindscribeService(const std::wstring &path_for_installation);
	void fixUnquotedServicePath();
};

#endif // SERVICE_H
