#include "applicationinfo.h"
#include "../../../common/version/windscribe_version.h"

ApplicationInfo::ApplicationInfo()
{

}

std::wstring ApplicationInfo::getName() const
{
	return name;
}

std::wstring ApplicationInfo::getVersion() const
{
	return WINDSCRIBE_VERSION_STR_UNICODE;
}

std::wstring ApplicationInfo::getUninstall() const
{
	return uninstall;
}

std::wstring  ApplicationInfo::getPublisher() const
{
	return publisher;
}

std::wstring ApplicationInfo::getId() const
{
	return id;
}

std::wstring ApplicationInfo::getPublisherUrl() const
{
	return publisherUrl;
}

std::wstring ApplicationInfo::getSupportUrl() const
{
	return supportURL;
}

std::wstring ApplicationInfo::getUpdateUrl() const
{
	return updateURL;
}

bool ApplicationInfo::appIsRunning()
{
    bool result = true;
    std::wstring classNameIcon = L"Qt631QWindowIcon";
    const std::wstring wsGuiIcon = L"Windscribe";

    HWND hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
    if (hwnd == NULL)
    {
        // Check if the old Qt 5.12 app is running.
        classNameIcon = L"Qt5QWindowIcon";
        hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
    }

    if (hwnd != NULL)
    {
        while (true)
        {
            int msgboxID = MessageBox(
                nullptr,
                static_cast<LPCWSTR>(L"Close Windscribe to continue. Please note, your connection will not be protected while the application is off.'"),
                static_cast<LPCWSTR>(L"Windscribe"),
                MB_ICONINFORMATION | MB_RETRYCANCEL);

            if (msgboxID == IDCANCEL)
            {
                // user clicked Cancel
                result = true;
                break;
            }
            else
            {
                hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
                if (hwnd == NULL)
                {
                    result = false;
                    break;
                }
            }
        }
    }
    else
    {
        result = false;
    }

    return result;
}
