#include "applicationinfo.h"

#include <memory>
#include <string>

#include "logger.h"
#include "path.h"
#include "../../../client/common/utils/win32handle.h"
#include "../../../client/common/version/windscribe_version.h"

class EnumWindowInfo
{
public:
	HWND appMainWindow = NULL;
};

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

static BOOL CALLBACK
FindAppWindowHandleProc(HWND hwnd, LPARAM lParam)
{
    DWORD processID = 0;
    ::GetWindowThreadProcessId(hwnd, &processID);

    if (processID == 0) {
        Log::instance().out(L"FindAppWindowHandleProc GetWindowThreadProcessId failed %lu", ::GetLastError());
        return TRUE;
    }

    wsl::Win32Handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID));
    if (!hProcess.isValid())
    {
        if (::GetLastError() != ERROR_ACCESS_DENIED) {
            Log::instance().out(L"FindAppWindowHandleProc OpenProcess failed %lu", ::GetLastError());
        }
        return TRUE;
    }

    TCHAR imageName[MAX_PATH];
    DWORD pathLen = sizeof(imageName) / sizeof(imageName[0]);
    BOOL result = ::QueryFullProcessImageName(hProcess.getHandle(), 0, imageName, &pathLen);

    if (result == FALSE) {
        Log::instance().out(L"FindAppWindowHandleProc QueryFullProcessImageName failed %lu", ::GetLastError());
        return TRUE;
    }

    std::wstring exeName = Path::PathExtractName(std::wstring(imageName, pathLen));

    if (_wcsicmp(exeName.c_str(), L"windscribe.exe") == 0)
    {
        TCHAR buffer[128];
        int resultLen = ::GetWindowText(hwnd, buffer, sizeof(buffer) / sizeof(buffer[0]));

        if (resultLen > 0 && (_wcsicmp(buffer, L"windscribe") == 0))
        {
            EnumWindowInfo* pWindowInfo = (EnumWindowInfo*)lParam;
            pWindowInfo->appMainWindow = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}

HWND ApplicationInfo::getAppMainWindowHandle()
{
    // Previously, we used FindWindow("Qt5QWindowIcon", "Windscribe") to find one of the app's
    // top-level window handles.  In Qt 6, the window class name became "QtNNNQWindowIcon, where
    // NNN is the Qt version (e.g. Qt631QWindowIcon).  This made it untenable to continue using
    // the FindWindow API as we change Qt versions, since we would have to search for all prior
    // Qt window class names.

    auto pWindowInfo = std::make_unique<EnumWindowInfo>();
    ::EnumWindows((WNDENUMPROC)FindAppWindowHandleProc, (LPARAM)pWindowInfo.get());

	return pWindowInfo->appMainWindow;
}
