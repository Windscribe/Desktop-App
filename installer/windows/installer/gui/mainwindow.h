#pragma once

#include <Windows.h>

#include "../Installer/installer.h"
#include "../messages.h"
#include "CloseButton.h"
#include "DesktopShortcutControl.h"
#include "EscButton.h"
#include "EulaButton.h"
#include "FactoryResetControl.h"
#include "InstallButton.h"
#include "MinimizeButton.h"
#include "PathControl.h"
#include "SettingsButton.h"
#include "TextItem.h"


class MainWindow
{
public:
	explicit MainWindow();
	virtual ~MainWindow();
	bool create(int windowCenterX, int windowCenterY);

	HWND getHwnd() { return hwnd_; }
	void drawBackground(Gdiplus::Graphics *graphics, int sx, int sy, int w, int h);
	void gotoAutoUpdateMode();
    void gotoSilentInstall();

private:
	HWND hwnd_ = nullptr;
	InstallButton *installButton_ = nullptr;
	CloseButton *closeButton_ = nullptr;
	MinimizeButton *minimizeButton_ = nullptr;
	SettingsButton *settingsButton_ = nullptr;
	EulaButton *eulaButton_ = nullptr;
	PathControl *pathControl_ = nullptr;
	DesktopShortcutControl *desktopShortcutControl_ = nullptr;
	FactoryResetControl *factoryResetControl_ = nullptr;
	EscButton *escButton_ = nullptr;

	Gdiplus::Bitmap *backgroundBitmap_ = nullptr;
	void createBackgroundBitmap(int width, int height);

	const int BIG_MARGIN = 16;
	const int SMALL_MARGIN = 8;

	const int OPACITY_TIMER_ID = 1235;

	TextItem *captionItem_ = nullptr;
	int topOffsTitle_ = 0;
	
	bool bMouseLeftButtonPressed_ = false;
	POINT startDragPosition_;

	bool compositionIsEnabled_ = false;
	bool themeIsEnabled_ = false;
	RECT rgn_;
	LONG winWidth_ = 0;
	LONG winHeight_ = 0;

	float curBackgroundOpacity_ = 1.0;
	bool incBackgroundOpacity_ = false;

	INSTALLER_CURRENT_STATE installerLastState_ = STATE_INIT;

    std::wstring strInstallTitle_;
    std::wstring strInstallButtonText_;

	static MainWindow *this_;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT onCreate(HWND hwnd);
	void onPaint();
	void redraw();
	void onMouseLeftButtonDown(int x, int y);
	void onMouseLeftButtonUp(int x, int y);
	void onMouseMove(int x, int y);

	void drawControl(HDC hdc);
    void resetControls();

	void onInstallClick(bool isUpdating);
	void onSettingsClick();
	void onEscapeClick();

	void onInstallerCallback(unsigned int progress, INSTALLER_CURRENT_STATE state);

	std::wstring selectPath();

	void onOpacityAnimationTimer();

	void handleCompositionChanged();
	void handleThemeChanged();
	void handleNCCalcSize(WPARAM wparam, LPARAM lparam);
	bool hasAutohideAppbar(UINT edge, RECT mon);
	LRESULT handleNCHittest(int x, int y);
	LRESULT handleMessageInvisible(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
	void handleWindowPosChanged(const WINDOWPOS *pos);
	void updateRegion();
};
