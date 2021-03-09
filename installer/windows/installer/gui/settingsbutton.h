#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class MainWindow;

class SettingsButton
{
public:
	SettingsButton(MainWindow *mainWindow);

	bool create(int x, int y, int w, int h);

	int getRecommendedWidth();
	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }

	void setEnabled(bool b);

private:
	MainWindow *mainWindow_;
	HWND hwnd_;

	bool bMouseTracking_;
	const int MARGIN = 7;

	bool bEnabled_;

	static SettingsButton *this_;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void onPaint();
	void redraw();
	void onMouseMove();
	void onMouseEnter();
	void onMouseLeave();
	void onLeftButtonDown();
	void onLeftButtonUp();

	void drawButton(HDC hdc);
};