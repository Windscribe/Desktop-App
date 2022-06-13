#pragma once
#include <Windows.h>
#include "InstallButton.h"
#include "CloseButton.h"
#include "MinimizeButton.h"
#include "SettingsButton.h"
#include "EulaButton.h"
#include "TextItem.h"
#include "PathControl.h"
#include "DesktopShortcutControl.h"
#include "FactoryResetControl.h"
#include "EscButton.h"
#include "../Installer/installer.h"
#include "../messages.h"


class MainWindow
{
public:
	explicit MainWindow(bool isLegacyOS);
	virtual ~MainWindow();
	bool create(int windowCenterX, int windowCenterY);

	HWND getHwnd() { return hwnd_; }
	void drawBackground(Gdiplus::Graphics *graphics, int sx, int sy, int w, int h);
	void gotoAutoUpdateMode();
    void gotoSilentInstall();

private:
	HWND hwnd_;
	InstallButton *instalButton_;
	CloseButton *closeButton_;
	MinimizeButton *minimizeButton_;
	SettingsButton *settingsButton_;
	EulaButton *eulaButton_;
	PathControl *pathControl_;
	DesktopShortcutControl *desktopShortcutControl_;
	FactoryResetControl *factoryResetControl_;
	EscButton *escButton_;

	Gdiplus::Bitmap *backgroundBitmap_;
	void createBackgroundBitmap(int width, int height);

	int progress_;

	const int BIG_MARGIN = 16;
	const int SMALL_MARGIN = 8;

	const int OPACITY_TIMER_ID = 1235;

	TextItem *captionItem_;
	int topOffsTitle_;
	
	bool bMouseLeftButtonPressed_;
	POINT startDragPosition_;

	bool compositionIsEnabled_;
	bool themeIsEnabled_;
	RECT rgn_;
	LONG winWidth_;
	LONG winHeight_;

	float curBackgroundOpacity_;
	bool incBackgroundOpacity_;

	INSTALLER_CURRENT_STATE installerLastState_;

    bool isLegacyOS_;
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
