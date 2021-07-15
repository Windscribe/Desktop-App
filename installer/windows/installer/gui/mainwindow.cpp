#include "MainWindow.h"
#include "Application.h"
#include "../resource.h"
#include "ImageResources.h"
#include "DPIScale.h"
#include "../../Utils/utils.h"
#include "../../Utils/applicationinfo.h"
#include <Windowsx.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <versionhelpers.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <gdiplus.h>

// #include <iostream>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Dwmapi.lib")

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

MainWindow *MainWindow::this_ = NULL;

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

MainWindow::MainWindow(bool isLegacyOS) : instalButton_(NULL), closeButton_(NULL), minimizeButton_(NULL), settingsButton_(NULL),
	eulaButton_(NULL), pathControl_(NULL), desktopShortcutControl_(NULL), escButton_(NULL),
	backgroundBitmap_(NULL), captionItem_(NULL), bMouseLeftButtonPressed_(false),
	curBackgroundOpacity_(1.0), installerLastState_(STATE_INIT), isLegacyOS_(isLegacyOS)
{
	memset(&rgn_, 0, sizeof(rgn_));
	this_ = this;
    if (isLegacyOS_) {
        strInstallTitle_ = L"Legacy Install";
        strInstallButtonText_ = L"Unsupported OS: Install legacy version.";
    }
    else {
        strInstallTitle_ = L"Install";
        strInstallButtonText_ = L"Click below to start.";
    }
}

MainWindow::~MainWindow()
{
	SAFE_DELETE(captionItem_);
	SAFE_DELETE(instalButton_);
	SAFE_DELETE(closeButton_);
	SAFE_DELETE(minimizeButton_);
	SAFE_DELETE(settingsButton_);
	SAFE_DELETE(eulaButton_);
	SAFE_DELETE(pathControl_);
	SAFE_DELETE(desktopShortcutControl_);
	SAFE_DELETE(escButton_);
	SAFE_DELETE(backgroundBitmap_);
}

bool MainWindow::create(int windowCenterX, int windowCenterY)
{
	instalButton_ = new InstallButton(this, strInstallTitle_.c_str());
	closeButton_ = new CloseButton(this);
	minimizeButton_ = new MinimizeButton(this);
    if (!isLegacyOS_)
        settingsButton_ = new SettingsButton(this);
	eulaButton_ = new EulaButton(this, L"Read EULA");
	pathControl_ = new PathControl(this);
	desktopShortcutControl_ = new DesktopShortcutControl(this);
	desktopShortcutControl_->setChecked(g_application->getSettings().getCreateShortcut());
	escButton_ = new EscButton(this);

	const int width_window = WINDOW_WIDTH;
	const int height_window = WINDOW_HEIGHT;
	TCHAR szTitle[] = L"Windscribe Installer";
	TCHAR szWindowClass[] = L"WindscribeInstaller_WC2";

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = NULL;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g_application->getInstance();
	wcex.hIcon = LoadIcon(g_application->getInstance(), MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(g_application->getInstance(), MAKEINTRESOURCE(IDI_ICON1));

	if (RegisterClassEx(&wcex) == NULL)
	{
		return false;
	}
	RECT clientRect = { 0, 0, width_window, height_window };
	AdjustWindowRect(&clientRect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, false);

	//Window at screen center
	//RECT wr = { 0, 0, width_window, height_window };    // set the size, but not the position
    const int cx = (windowCenterX >= 0) ? windowCenterX : (GetSystemMetrics(SM_CXSCREEN) >> 1);
    const int cy = (windowCenterY >= 0) ? windowCenterY : (GetSystemMetrics(SM_CYSCREEN) >> 1);
    const int x = cx - (width_window >> 1);
    const int y = cy - (height_window >> 1);
    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;

	createBackgroundBitmap(clientRect.right, clientRect.bottom);

	hwnd_ = CreateWindowEx(WS_EX_APPWINDOW /*| WS_EX_LAYERED*/, szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
		x, y, width, height, nullptr, nullptr, g_application->getInstance(), nullptr);
	if (!hwnd_)
	{
		return false;
	}
	///SetLayeredWindowAttributes(hwnd_, RGB(255, 0, 255), 0, LWA_COLORKEY);
	//SetLayeredWindowAttributes(hwnd_, 0, (255 * 70) / 100, LWA_ALPHA);

	handleCompositionChanged();
	handleThemeChanged();

	ShowWindow(hwnd_, g_application->getCmdShow());
	UpdateWindow(hwnd_);
	
	return true;
}

void MainWindow::gotoAutoUpdateMode()
{
	onInstallClick(true);
}

void MainWindow::createBackgroundBitmap(int width, int height)
{
	SAFE_DELETE(backgroundBitmap_);
	backgroundBitmap_ = new Gdiplus::Bitmap(width, height);
	Gdiplus::Graphics *g = Gdiplus::Graphics::FromImage(backgroundBitmap_);
	g->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

	Gdiplus::Bitmap *img = g_application->getImageResources()->getBackgroundImage()->getBitmap();
	if (img)
	{
		g->DrawImage(img, Gdiplus::REAL(width - img->GetWidth()) / 2, Gdiplus::REAL(height - img->GetHeight()) / 2);
	}

	delete g;
}

void MainWindow::drawBackground(Gdiplus::Graphics *graphics, int sx, int sy, int w, int h)
{
	graphics->FillRectangle(g_application->getImageResources()->getBackgroundBrush(), -1, -1, w + 1, h + 1);

	Gdiplus::ColorMatrix matrix =
	{
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 0, curBackgroundOpacity_, 0,
		0, 0, 0, 0, 1
	};
	Gdiplus::ImageAttributes attrib;
	attrib.SetColorMatrix(&matrix);
	graphics->DrawImage(backgroundBitmap_, Gdiplus::Rect(0, 0, w, h), sx, sy, w, h, Gdiplus::UnitPixel, &attrib);
}

LRESULT CALLBACK  MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return this_->realWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK  MainWindow::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	std::wstring path;

	switch (msg)
	{
		case WM_CREATE:
			return onCreate(hwnd);
		case WM_CLOSE:
			if (installerLastState_ == STATE_EXTRACTING)
			{
				g_application->getInstaller()->pause();
				g_application->getInstaller()->cancel();
				DestroyWindow(hwnd_);
			}
			else
			{
				DestroyWindow(hwnd_);
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_ERASEBKGND:
			return 1;
		case WM_DWMCOMPOSITIONCHANGED:
			handleCompositionChanged();
			return 0;
		case WM_NCACTIVATE:
			// DefWindowProc won't repaint the window border if lParam (normally a
			//   HRGN) is -1. This is recommended in:
			//   https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/ 
			return DefWindowProc(hwnd, msg, wParam, -1);
		case WM_NCCALCSIZE:
			handleNCCalcSize(wParam, lParam);
			return 0;

		case WM_NCHITTEST:
			return handleNCHittest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		case WM_NCPAINT:
			// Only block WM_NCPAINT when composition is disabled. If it's blocked
			//   when composition is enabled, the window shadow won't be drawn. 
			if (!compositionIsEnabled_)
			{
				return 0;
			}
			break;
		case WM_NCUAHDRAWCAPTION:
		case WM_NCUAHDRAWFRAME:
			// These undocumented messages are sent to draw themed window borders.
			//   Block them to prevent drawing borders over the client area. 
			return 0;
		case WM_SETICON:
		case WM_SETTEXT:
			// Disable painting while these messages are handled to prevent them
			//   from drawing a window caption over the client area, but only when
			//   composition and theming are disabled. These messages don't paint
			//   when composition is enabled and blocking WM_NCUAHDRAWCAPTION should
			//   be enough to prevent painting when theming is enabled. 
			if (!compositionIsEnabled_ && !themeIsEnabled_)
				return handleMessageInvisible(hwnd, msg, wParam, lParam);
			break;
		case WM_THEMECHANGED:
			handleThemeChanged();
			break;
		case WM_WINDOWPOSCHANGED:
			handleWindowPosChanged((WINDOWPOS*)lParam);
			return 0;
			
		case WM_PAINT:
			onPaint();
			return 0;
		case WM_LBUTTONDOWN:
			onMouseLeftButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
			onMouseLeftButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_MOUSEMOVE:
			onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WI_CLICK_CLOSE:
			SendMessage(hwnd_, WM_CLOSE, NULL, NULL);
			return 0;

		case WI_CLICK_MINIMIZE:
			ShowWindow(hwnd_, SW_SHOWMINIMIZED);
			return 0;

		case WI_CLICK_SETTINGS:
			onSettingsClick();
			return 0;

		case WI_CLICK_ESCAPE:
			onEscapeClick();
			return 0;

		case WI_CLICK_INSTALL:
			onInstallClick(false);
			return 0;

		case WM_TIMER:
			if (wParam == OPACITY_TIMER_ID)
			{
				onOpacityAnimationTimer();
			}

			return 0;

		case WI_CLICK_EULA:
			ShellExecute(0, 0, L"https://windscribe.com/terms/eula", 0, 0, SW_SHOW);
			return 0;

		case WI_CLICK_SELECT_PATH:
			path = selectPath();
			if (!path.empty())
			{
				pathControl_->setPath(path);
			}
			return 0;

		case WI_INSTALLER_STATE:
			onInstallerCallback(wParam, (INSTALLER_CURRENT_STATE)lParam);
			return 0;
	};
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::onCreate(HWND hwnd)
{
	hwnd_ = hwnd;
	
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);

	int wInstall = instalButton_->getRecommendedWidth();
	int hInstall = instalButton_->getRecommendedHeight();
	int xInstall = (clientRect.right - wInstall) / 2;
	int yInstall = (clientRect.bottom - hInstall) / 2 + BIG_MARGIN * SCALE_FACTOR;

	if (!instalButton_->create(xInstall, yInstall, wInstall, hInstall))
	{
		return -1;
	}

	captionItem_ = new TextItem(strInstallButtonText_.c_str(), (int)(16 * SCALE_FACTOR), true);
	topOffsTitle_ = yInstall - BIG_MARGIN * SCALE_FACTOR - captionItem_->textHeight();

	const int CLOSE_BUTTON_MARGIN = (int)(10 * SCALE_FACTOR);
	if (!closeButton_->create(clientRect.right - closeButton_->getRecommendedWidth() - CLOSE_BUTTON_MARGIN, CLOSE_BUTTON_MARGIN, closeButton_->getRecommendedWidth(), closeButton_->getRecommendedHeight()))
	{
		return -1;
	}

	if (!minimizeButton_->create(clientRect.right - closeButton_->getRecommendedWidth() - (int)(CLOSE_BUTTON_MARGIN * 2.5) - minimizeButton_->getRecommendedWidth(), 
								 CLOSE_BUTTON_MARGIN, 
								 minimizeButton_->getRecommendedWidth(), minimizeButton_->getRecommendedHeight()))
	{
		return -1;
	}

	int wEula = eulaButton_->getRecommendedWidth();
	int hEula = eulaButton_->getRecommendedHeight();
	int xEula = (clientRect.right - wEula) / 2;
	int yEula = clientRect.bottom - hEula - BIG_MARGIN * SCALE_FACTOR;
	if (!eulaButton_->create(xEula, yEula, wEula, hEula))
	{
		return -1;
	}

    if (settingsButton_) {
        int wSettings = settingsButton_->getRecommendedWidth();
        int hSettings = settingsButton_->getRecommendedHeight();
        int xSettings = (clientRect.right - wSettings) / 2;
        int ySettings = clientRect.bottom - hSettings - BIG_MARGIN * SCALE_FACTOR;

        if (!settingsButton_->create(xSettings, yEula - hSettings - SMALL_MARGIN * SCALE_FACTOR,
                                     wSettings, hSettings))
        {
            return -1;
        }
    }

	int settingsOffs = 16 * SCALE_FACTOR;
	if (!pathControl_->create(settingsOffs, yInstall - 7 * SCALE_FACTOR, clientRect.right - settingsOffs, pathControl_->getRecommendedHeight(), g_application->getSettings().getPath()))
	{
		return -1;
	}

	if (!desktopShortcutControl_->create(settingsOffs, yInstall - 7 * SCALE_FACTOR + pathControl_->getRecommendedHeight(), clientRect.right - settingsOffs, desktopShortcutControl_->getRecommendedHeight()))
	{
		return -1;
	}

	int wEsc = escButton_->getRecommendedWidth();
	int hEsc = escButton_->getRecommendedHeight();
	int xEsc = (clientRect.right - wEsc) / 2;
	int yEsc = clientRect.bottom - hEsc - BIG_MARGIN * SCALE_FACTOR;
	if (!escButton_->create(xEsc, yEsc, wEsc, hEsc))
	{
		return -1;
	}

	//PostMessage(hwnd_, WI_CLICK_SETTINGS, 0, 0);

	return 0;
}

void MainWindow::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd_, &ps);
	drawControl(hdc);
	EndPaint(hwnd_, &ps);
}

void MainWindow::redraw()
{
	HDC hdc = GetDC(hwnd_);
	drawControl(hdc);
	ReleaseDC(hwnd_, hdc);
}

void MainWindow::onMouseLeftButtonDown(int x, int y)
{
	bMouseLeftButtonPressed_ = true;
	SetCapture(hwnd_);
	POINT pt = { x, y };
	ClientToScreen(hwnd_, &pt);

	RECT rc;
	GetWindowRect(hwnd_, &rc);
	startDragPosition_.x = pt.x - rc.left;
	startDragPosition_.y = pt.y - rc.top;
}

void MainWindow::onMouseLeftButtonUp(int x, int y)
{
	bMouseLeftButtonPressed_ = false;
	ReleaseCapture();
}

void MainWindow::onMouseMove(int x, int y)
{
	if (bMouseLeftButtonPressed_)
	{
		POINT pt = { x, y };
		ClientToScreen(hwnd_, &pt);
		SetWindowPos(hwnd_, NULL, pt.x - startDragPosition_.x, pt.y - startDragPosition_.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

void MainWindow::drawControl(HDC hdc)
{
	RECT clientRect;
	GetClientRect(hwnd_, &clientRect);

	Gdiplus::Bitmap doubleBufferBmp(clientRect.right, clientRect.bottom);
	Gdiplus::Graphics *graphics = Gdiplus::Graphics::FromImage(&doubleBufferBmp);
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	drawBackground(graphics, 0, 0, backgroundBitmap_->GetWidth(), backgroundBitmap_->GetHeight());

	// draw logo icon
	Gdiplus::REAL xLogo = (clientRect.right - g_application->getImageResources()->getBadgeIcon()->GetWidth()) / 2;
	Gdiplus::REAL yLogo = (Gdiplus::REAL)(16 * SCALE_FACTOR);
	float margin = 3 * SCALE_FACTOR;
	/*graphics->FillPie(g_application->getImageResources()->getBackgroundBrush(), xLogo + margin, yLogo + margin,
		g_application->getImageResources()->getBadgeIcon()->GetWidth() - margin * 2.0f,
		g_application->getImageResources()->getBadgeIcon()->GetHeight() - margin * 2.0f, 0.0f, 360.0f);*/
	graphics->DrawImage(g_application->getImageResources()->getBadgeIcon(), xLogo, yLogo);

	// draw caption with shadow
	{
		Gdiplus::SolidBrush brush(Gdiplus::Color(0, 0, 0));
		Gdiplus::StringFormat strformat;
		graphics->DrawString(captionItem_->text(), wcslen(captionItem_->text()), captionItem_->getFont(),
			Gdiplus::PointF((Gdiplus::REAL)((clientRect.right - captionItem_->textWidth()) / 2 + 1.0f), (Gdiplus::REAL)(topOffsTitle_ + 1.0f)), &strformat, &brush);
	}
	{
		Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255));
		Gdiplus::StringFormat strformat;
		graphics->DrawString(captionItem_->text(), wcslen(captionItem_->text()), captionItem_->getFont(),
			Gdiplus::PointF((Gdiplus::REAL)((clientRect.right - captionItem_->textWidth()) / 2), (Gdiplus::REAL)(topOffsTitle_)), &strformat, &brush);
	}

	delete graphics;

	// draw on screen
	{
		Gdiplus::Graphics screenGraphics(hdc);
		screenGraphics.DrawImage(&doubleBufferBmp, 0, 0);
	}
}

void MainWindow::resetControls()
{
    ShowWindow(instalButton_->getHwnd(), SW_SHOW);
    ShowWindow(eulaButton_->getHwnd(), SW_SHOW);
    if (!isLegacyOS_)
        ShowWindow(settingsButton_->getHwnd(), SW_SHOW);

    ShowWindow(pathControl_->getHwnd(), SW_HIDE);
    ShowWindow(desktopShortcutControl_->getHwnd(), SW_HIDE);
    ShowWindow(escButton_->getHwnd(), SW_HIDE);

    instalButton_->setState(InstallButton::INSTALL_TITLE);
    captionItem_->changeText(strInstallButtonText_.c_str());
    redraw();
}

void MainWindow::onInstallClick(bool isUpdating)
{
	/*if (MessageBox(hwnd_, L"This is a beta preview release that is not meant to be used in critical environments. There are known and unknown bugs, so use this at your own risk.", 
		L"Windscribe Installer", MB_OKCANCEL | MB_ICONINFORMATION) == IDCANCEL)
	{
		return;
	}*/

	if (isUpdating)
	{
		captionItem_->changeText(L"Updating...");
	}
    else if (isLegacyOS_)
    {
        captionItem_->changeText(L"Downloading legacy installer...");
    }
	else
	{
		captionItem_->changeText(L"Installing...");
	}
    if (settingsButton_)
        settingsButton_->setEnabled(false);
	instalButton_->setState(InstallButton::WAIT_WITH_PROGRESS);
	InvalidateRect(hwnd_, NULL, TRUE);
	UpdateWindow(hwnd_);

	g_application->getSettings().setPath(pathControl_->path());
	g_application->getSettings().setCreateShortcut(desktopShortcutControl_->getChecked());

	g_application->getInstaller()->start(hwnd_, g_application->getSettings());
}

void MainWindow::onSettingsClick()
{
	incBackgroundOpacity_ = false;
	SetTimer(hwnd_, OPACITY_TIMER_ID, 10, NULL);

	ShowWindow(instalButton_->getHwnd(), SW_HIDE);
	ShowWindow(eulaButton_->getHwnd(), SW_HIDE);
    if (settingsButton_)
        ShowWindow(settingsButton_->getHwnd(), SW_HIDE);

	ShowWindow(pathControl_->getHwnd(), SW_SHOW);
	ShowWindow(desktopShortcutControl_->getHwnd(), SW_SHOW);
	ShowWindow(escButton_->getHwnd(), SW_SHOW);

	captionItem_->changeText(L"Install Settings");
	redraw();
}

void MainWindow::onEscapeClick()
{
	incBackgroundOpacity_ = true;
	SetTimer(hwnd_, OPACITY_TIMER_ID, 10, NULL);
    resetControls();
}

void MainWindow::onInstallerCallback(unsigned int progress, INSTALLER_CURRENT_STATE state)
{
	installerLastState_ = state;

	if (state == STATE_EXTRACTING)
	{
		instalButton_->setProgress(progress);
	}
	else if (state == STATE_FINISHED)
	{
		instalButton_->setProgress(progress);
		captionItem_->changeText(L"Launching...");
		InvalidateRect(hwnd_, NULL, TRUE);
		UpdateWindow(hwnd_);

		g_application->getInstaller()->runLauncher();
		DestroyWindow(hwnd_);
	}
	else if (state == STATE_ERROR || state == STATE_FATAL_ERROR)
	{
		MessageBox(hwnd_, g_application->getInstaller()->getLastError().c_str(), L"Error", MB_OK | MB_ICONINFORMATION);
        if (state == STATE_FATAL_ERROR) {
            DestroyWindow(hwnd_);
        } else {
            g_application->getInstaller()->cancel();
            resetControls();
        }
	}
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	if (uMsg == BFFM_INITIALIZED)
	{
		std::wstring tmp = (const wchar_t *)lpData;
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

std::wstring MainWindow::selectPath()
{
	TCHAR path[MAX_PATH];

	std::wstring path_param = pathControl_->path();

	BROWSEINFO bi = { 0 };
	bi.hwndOwner = hwnd_;
	bi.lpszTitle = (L"Select a folder in the list below, then click OK.");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param.c_str();

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		//get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		//free memory used
		IMalloc * imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}

		if (PathIsSystemFolder(path, 0))
		{
			if (PathAppend(path, ApplicationInfo::instance().getName().c_str()))
			{
				return path;
			}
			else
			{
				return L"";
			}
		}
		return path;
	}

	return L"";
}

void MainWindow::onOpacityAnimationTimer()
{
	const float step = 0.05f;
	if (!incBackgroundOpacity_)
	{
		curBackgroundOpacity_ -= step;
		if (curBackgroundOpacity_ < 0.3f)
		{
			curBackgroundOpacity_ = 0.3f;
			KillTimer(hwnd_, OPACITY_TIMER_ID);
		}
	}
	else
	{
		curBackgroundOpacity_ += step;
		if (curBackgroundOpacity_ > 1.0f)
		{
			curBackgroundOpacity_ = 1.0f;
			KillTimer(hwnd_, OPACITY_TIMER_ID);
		}
	}
	redraw();
	SendMessage(pathControl_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
	SendMessage(desktopShortcutControl_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
	SendMessage(instalButton_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
    if (settingsButton_)
        SendMessage(settingsButton_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
	SendMessage(eulaButton_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
	SendMessage(escButton_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
	SendMessage(minimizeButton_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
	SendMessage(closeButton_->getHwnd(), WI_FORCE_REDRAW, 0, 0);
}

void MainWindow::handleCompositionChanged()
{
	BOOL enabled = FALSE;
	DwmIsCompositionEnabled(&enabled);
	compositionIsEnabled_ = enabled;

	if (enabled) 
	{
		// The window needs a frame to show a shadow, so give it the smallest
		// amount of frame possible 
		MARGINS margins{ 0, 0, 1, 0 };
		DwmExtendFrameIntoClientArea(hwnd_, &margins);
		DWORD dwAttribute = DWMNCRP_ENABLED;
		DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY,
			&dwAttribute, sizeof(DWORD));
	}

	updateRegion();

}
void MainWindow::handleThemeChanged()
{
	themeIsEnabled_ = IsThemeActive();
}

void MainWindow::handleNCCalcSize(WPARAM wparam, LPARAM lparam)
{
	union {
		LPARAM lparam;
		RECT* rect;
	} params;
	params.lparam = lparam;

	// DefWindowProc must be called in both the maximized and non-maximized
	//   cases, otherwise tile/cascade windows won't work
	RECT nonclient = *params.rect;
	DefWindowProc(hwnd_, WM_NCCALCSIZE, wparam, params.lparam);
	RECT client = *params.rect;

	if (IsMaximized(hwnd_)) {
		WINDOWINFO wi = {};
		wi.cbSize = sizeof(wi);
		GetWindowInfo(hwnd_, &wi);

		// Maximized windows always have a non-client border that hangs over
		// the edge of the screen, so the size proposed by WM_NCCALCSIZE is
		// fine. Just adjust the top border to remove the window title.
		*params.rect = {
			client.left,
			nonclient.top + (LONG)wi.cyWindowBorders,
			client.right,
			client.bottom,
		};

		HMONITOR mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {};
		mi.cbSize = sizeof(mi);
		GetMonitorInfoW(mon, &mi);

		// If the client rectangle is the same as the monitor's rectangle,
		//   the shell assumes that the window has gone fullscreen, so it removes
		//   the topmost attribute from any auto-hide appbars, making them
		//   inaccessible. To avoid this, reduce the size of the client area by
		//   one pixel on a certain edge. The edge is chosen based on which side
		//   of the monitor is likely to contain an auto-hide appbar, so the
		//   missing client area is covered by it.
		if (EqualRect(params.rect, &mi.rcMonitor)) {
			if (hasAutohideAppbar(ABE_BOTTOM, mi.rcMonitor))
				params.rect->bottom--;
			else if (hasAutohideAppbar(ABE_LEFT, mi.rcMonitor))
				params.rect->left++;
			else if (hasAutohideAppbar(ABE_TOP, mi.rcMonitor))
				params.rect->top++;
			else if (hasAutohideAppbar(ABE_RIGHT, mi.rcMonitor))
				params.rect->right--;
		}
	}
	else {
		// For the non-maximized case, set the output RECT to what it was
		//   before WM_NCCALCSIZE modified it. This will make the client size the
		//   same as the non-client size.
		*params.rect = nonclient;
	}

}
bool MainWindow::hasAutohideAppbar(UINT edge, RECT mon)
{
	if (IsWindows8Point1OrGreater()) 
	{
		APPBARDATA data = {};
		data.cbSize = sizeof(APPBARDATA);
		data.uEdge = edge;
		data.rc = mon;
		return SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data);
	}

	// Before Windows 8.1, it was not possible to specify a monitor when
	//   checking for hidden appbars, so check only on the primary monitor
	if (mon.left != 0 || mon.top != 0)
	{
		return false;
	}

	APPBARDATA data = {};
	data.cbSize = sizeof(APPBARDATA);
	data.uEdge = edge;

	return SHAppBarMessage(ABM_GETAUTOHIDEBAR, &data);
}

LRESULT MainWindow::handleNCHittest(int x, int y)
{
	if (IsMaximized(hwnd_))
		return HTCLIENT;

	POINT mouse = { x, y };
	ScreenToClient(hwnd_, &mouse);

	// The horizontal frame should be the same size as the vertical frame,
	//   since the NONCLIENTMETRICS structure does not distinguish between them 
	int frame_size = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
	// The diagonal size handles are wider than the frame
	int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

	if (mouse.y < frame_size) {
		if (mouse.x < diagonal_width)
			return HTTOPLEFT;
		if (mouse.x >= winWidth_ - diagonal_width)
			return HTTOPRIGHT;
		return HTTOP;
	}

	if (mouse.y >= winHeight_ - frame_size) {
		if (mouse.x < diagonal_width)
			return HTBOTTOMLEFT;
		if (mouse.x >= winWidth_ - diagonal_width)
			return HTBOTTOMRIGHT;
		return HTBOTTOM;
	}

	if (mouse.x < frame_size)
		return HTLEFT;
	if (mouse.x >= winWidth_ - frame_size)
		return HTRIGHT;
	return HTCLIENT;
}

LRESULT MainWindow::handleMessageInvisible(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LONG_PTR old_style = GetWindowLongPtrW(window, GWL_STYLE);

	// Prevent Windows from drawing the default title bar by temporarily
	//   toggling the WS_VISIBLE style. This is recommended in:
	//   https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/
	SetWindowLongPtrW(window, GWL_STYLE, old_style & ~WS_VISIBLE);
	LRESULT result = DefWindowProcW(window, msg, wparam, lparam);
	SetWindowLongPtrW(window, GWL_STYLE, old_style);

	return result;
}

void MainWindow::handleWindowPosChanged(const WINDOWPOS *pos)
{
	RECT client;
	GetClientRect(hwnd_, &client);
	LONG old_width = winWidth_;
	LONG old_height = winHeight_;
	winWidth_ = client.right;
	winHeight_ = client.bottom;
	bool client_changed = winWidth_ != old_width || winHeight_ != old_height;

	if (client_changed || (pos->flags & SWP_FRAMECHANGED))
	{
		updateRegion();
	}

	if (client_changed) 
	{
		// Invalidate the changed parts of the rectangle drawn in WM_PAINT
		if (winWidth_ > old_width) 
		{
			RECT rc = { old_width - 1, 0, old_width, old_height };
			InvalidateRect(hwnd_, &rc, TRUE);
		}
		else 
		{
			RECT rc = { winWidth_ - 1, 0, winWidth_, winHeight_ };
			InvalidateRect(hwnd_, &rc, TRUE);
		}
		if (winHeight_ > old_height) 
		{
			RECT rc = { 0, old_height - 1, old_width, old_height };
			InvalidateRect(hwnd_, &rc, TRUE);
		}
		else 
		{
			RECT rc = { 0, winHeight_ - 1, winWidth_, winHeight_ };
			InvalidateRect(hwnd_, &rc, TRUE);
		}
	}
}

void MainWindow::updateRegion()
{
	RECT old_rgn = rgn_;

	if (IsMaximized(hwnd_)) 
	{
		WINDOWINFO wi;
		memset(&wi, 0, sizeof(wi));
		wi.cbSize = sizeof(wi);
		GetWindowInfo(hwnd_, &wi);

		// For maximized windows, a region is needed to cut off the non-client
		// borders that hang over the edge of the screen 
		rgn_.left = wi.rcClient.left - wi.rcWindow.left;
		rgn_.top = wi.rcClient.top - wi.rcWindow.top;
		rgn_.right = wi.rcClient.right - wi.rcWindow.left;
		rgn_.bottom = wi.rcClient.bottom - wi.rcWindow.top;
	}
	else if (!compositionIsEnabled_) 
	{
		// For ordinary themed windows when composition is disabled, a region
		//   is needed to remove the rounded top corners. Make it as large as
		//   possible to avoid having to change it when the window is resized.
		rgn_.left = 0;
		rgn_.top = 0;
		rgn_.right = 32767;
		rgn_.bottom = 32767;
	}
	else {
		// Don't mess with the region when composition is enabled and the
		//   window is not maximized, otherwise it will lose its shadow
		rgn_.left = 0;
		rgn_.top = 0;
		rgn_.right = 0;
		rgn_.bottom = 0;
	}

	// Avoid unnecessarily updating the region to avoid unnecessary redraws
	if (EqualRect(&rgn_, &old_rgn))
		return;
	// Treat empty regions as NULL regions
	RECT nullRc = { 0 };
	if (EqualRect(&rgn_, &nullRc))
		SetWindowRgn(hwnd_, NULL, TRUE);
	else
		SetWindowRgn(hwnd_, CreateRectRgnIndirect(&rgn_), TRUE);

}
