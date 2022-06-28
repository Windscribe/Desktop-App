#ifndef UNINSTALLPREV_H
#define UNINSTALLPREV_H
#include <string>

#include "../iinstall_block.h"
#include "../../../Utils/services.h"


class UninstallPrev : public IInstallBlock
{
public:
	virtual int executeStep();
	UninstallPrev(bool isFactoryReset, double weight);

private:
	Services services;
	int state_;
	bool isFactoryReset_;
	
	std::wstring getUninstallString();
	int uninstallOldVersion(const std::wstring &uninstallString);
	std::wstring removeQuotes(const std::wstring &str);
	void doFactoryReset();
};

#endif // UNINSTALLPREV_H
