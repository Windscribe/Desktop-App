#ifndef UNINSTALLPREV_H
#define UNINSTALLPREV_H
#include <string>

#include "../iinstall_block.h"
#include "../../../Utils/services.h"


class UninstallPrev : public IInstallBlock
{
public:
	virtual int executeStep();
	UninstallPrev(double weight);

private:
	Services services;
	int state_;
	
	std::wstring getUninstallString();
	int uninstallOldVersion(const std::wstring &uninstallString);
	std::wstring removeQuotes(const std::wstring &str);
};

#endif // UNINSTALLPREV_H
