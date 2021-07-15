#include "Application.h"
#include "ImageResources.h"
#include "../../Utils/applicationinfo.h"
#include "../installer/downloader.h"
#include "../installer/installer.h"
#include <shlobj_core.h>
#include <VersionHelpers.h>

#pragma comment(lib, "gdiplus.lib")


Application *g_application = NULL;

Application::Application(HINSTANCE hInstance, int nCmdShow, bool isAutoUpdateMode) :
	hInstance_(hInstance),
	nCmdShow_(nCmdShow),
	isAutoUpdateMode_(isAutoUpdateMode),
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

	if (!isAutoUpdateMode || !settings_.readFromRegistry())
	{
		// set default install path
		TCHAR programFilesPath[MAX_PATH];
		SHGetSpecialFolderPath(0, programFilesPath, CSIDL_PROGRAM_FILES, FALSE);
		std::wstring installPath = std::wstring(programFilesPath) + L"\\" + ApplicationInfo::instance().getName();
		settings_.setPath(installPath);
	}

	imageResources_ = new ImageResources();
	fontResources_ = new FontResources();
	mainwWindow_ = new MainWindow(isLegacyOS_);
}

Application::~Application()
{
	g_application = NULL;

	delete mainwWindow_;
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

	bool bSuccess = mainwWindow_->create(windowCenterX, windowCenterY);

	if (bSuccess && isAutoUpdateMode_)
	{
		mainwWindow_->gotoAutoUpdateMode();
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
	PostMessage(mainwWindow_->getHwnd(), WI_INSTALLER_STATE, progress, state);
}
