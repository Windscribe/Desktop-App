#include "settings.h"

#include "../../utils/path.h"
#include "../../utils/registry.h"

Settings::Settings() : isCreateShortcut_(true)
{
}

void Settings::setPath(const std::wstring &path)
{
	path_ = Path::RemoveBackslashUnlessRoot(path);
}

std::wstring Settings::getPath() const
{
	return path_;
}

void Settings::setCreateShortcut(bool create_shortcut)
{
	isCreateShortcut_ = create_shortcut;
}

bool Settings::getCreateShortcut() const
{
	return isCreateShortcut_;
}

void Settings::setInstallDrivers(bool install)
{
	isInstallDrivers_ = install;
}

bool Settings::getInstallDrivers() const
{
	return isInstallDrivers_;
}

void Settings::setAutoStart(bool autostart)
{
	isAutoStart_ = autostart;
}

bool Settings::getAutoStart() const
{
	return isAutoStart_;
}

void Settings::setFactoryReset(bool erase)
{
	isFactoryReset_ = erase;
}

bool Settings::getFactoryReset() const
{
	return isFactoryReset_;
}

bool Settings::readFromRegistry()
{
	bool bRet = true;
	std::wstring tempStr;

	if (Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"applicationPath", tempStr))
	{
		path_ = tempStr;
	}
	else
	{
		bRet = false;
	}

	if (Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"isCreateShortcut", tempStr))
	{
		if (!tempStr.empty())
		{
			isCreateShortcut_ = (tempStr[0] != L'0');
		}
		else
		{
			isCreateShortcut_ = true;
		}
	}
	else
	{
		bRet = false;
	}

	if (Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"isFactoryReset", tempStr))
	{
		isFactoryReset_ = true;
	}
	else
	{
		bRet = false;
	}

	return bRet;
}

void Settings::writeToRegistry() const
{
	Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"applicationPath", path_.c_str());
	Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"isCreateShortcut", isCreateShortcut_ ? L"1" : L"0");
	Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"isFactoryReset", isFactoryReset_ ? L"1" : L"0");
}
