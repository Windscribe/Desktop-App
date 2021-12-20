#include "settings.h"
#include "../../Utils/registry.h"

Settings::Settings() : isCreateShortcut_(true)
{
}

void Settings::setPath(const std::wstring &path)
{
	path_ = path;
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

bool Settings::readFromRegistry()
{
	bool bRet1 = false;
	bool bRet2 = false;
	std::wstring tempStr;
	if (Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"applicationPath", tempStr))
	{
		path_ = tempStr;
		bRet1 = true;
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
		bRet2 = true;
	}
	return bRet1 && bRet2;
}

void Settings::writeToRegistry() const
{
	Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"applicationPath", path_.c_str());
	Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Installer", L"isCreateShortcut", isCreateShortcut_ ? L"1" : L"0");
}

