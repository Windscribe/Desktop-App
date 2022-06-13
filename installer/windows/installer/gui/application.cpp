#include "Application.h"
#include "ImageResources.h"
#include "../../Utils/applicationinfo.h"
#include "../installer/downloader.h"
#include "../installer/installer.h"
#include <shlobj_core.h>
#include <VersionHelpers.h>
#include "../../utils/directory.h"
#include "../../utils/registry.h"

#pragma comment(lib, "gdiplus.lib")


Application *g_application = NULL;

Application::Application(HINSTANCE hInstance, int nCmdShow, bool isAutoUpdateMode,
                         bool isSilent, bool noDrivers, bool noAutoStart, bool isFactoryReset,
						 const std::wstring& installPath) :
	hInstance_(hInstance),
	nCmdShow_(nCmdShow),
	isAutoUpdateMode_(isAutoUpdateMode),
    isSilent_(isSilent),
    isLegacyOS_(!IsWindows7OrGreater())
{
	g_application = this;

    auto callback = std::bind(&Application::installerCallback, this,
                              std::placeholders::_1, std::placeholders::_2);
    if (isLegacyOS_)
        installer_.reset(new Downloader(callback));
    else
        installer_.reset(new Installer(callback));

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	if (GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
	{
		gdiplusToken_ = NULL;
	}

    settings_.readFromRegistry();

    if (!isAutoUpdateMode_ && !installPath.empty()) {
        settings_.setPath(installPath);
    }
    else if (settings_.getPath().empty() || !Directory::DirExists(settings_.getPath()))
    {
        // We don't have an install folder specified in the Registry, or the folder specified
        // in the Registry no longer exists, indicating the user uninstalled the app.
        TCHAR programFilesPath[MAX_PATH];
        SHGetSpecialFolderPath(0, programFilesPath, CSIDL_PROGRAM_FILES, FALSE);
        std::wstring defaultInstallPath = std::wstring(programFilesPath) + L"\\" + ApplicationInfo::instance().getName();
        settings_.setPath(defaultInstallPath);
    }

    settings_.setInstallDrivers(!isSilent_ && !noDrivers);
    settings_.setAutoStart(!isSilent_ && !noAutoStart);
    settings_.setFactoryReset(isFactoryReset);

	imageResources_ = new ImageResources();
	fontResources_ = new FontResources();
	mainWindow_ = new MainWindow(isLegacyOS_);
}

Application::~Application()
{
	g_application = NULL;

	delete mainWindow_;
	delete fontResources_;
	delete imageResources_;

	if (gdiplusToken_)
	{
		Gdiplus::GdiplusShutdown(gdiplusToken_);
	}
}

bool Application::init(int windowCenterX, int windowCenterY)
{
	if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) != S_OK)
	{
		return false;
	}

	if (gdiplusToken_ == NULL)
	{
		return false;
	}

	if (!imageResources_->init())
	{
		return false;
	}
	
	if (!fontResources_->init())
	{
		return false;
	}

	bool bSuccess = mainWindow_->create(windowCenterX, windowCenterY);

	if (bSuccess)
	{
        if (isAutoUpdateMode_) {
            mainWindow_->gotoAutoUpdateMode();
        }
        else if (isSilent_) {
            mainWindow_->gotoSilentInstall();
        }
	}

	return bSuccess;
}

int Application::exec()
{
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	settings_.writeToRegistry();

	return static_cast<int> (msg.wParam);
}

void Application::installerCallback(unsigned int progress, INSTALLER_CURRENT_STATE state)
{
	// transform to window messages
	PostMessage(mainWindow_->getHwnd(), WI_INSTALLER_STATE, progress, state);
}


// return path of installed Windscribe app or empty string if not installed
std::wstring Application::getPreviousInstallPath()
{
	HKEY rootKey = HKEY_LOCAL_MACHINE;
	std::wstring subkeyName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ApplicationInfo::instance().getId() + L"_is1";

	bool bRet = false;
	HKEY hKey;
	if (Registry::RegOpenKeyExView(rvDefault, rootKey, subkeyName.c_str(), 0, KEY_QUERY_VALUE, hKey) == ERROR_SUCCESS)
	{
		std::wstring path;
		if (Registry::RegQueryStringValue1(hKey, L"InstallLocation", path))
		{
			return path;
		}
		RegCloseKey(hKey);
	}
	return L"";

}