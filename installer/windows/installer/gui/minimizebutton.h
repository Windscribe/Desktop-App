#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class MainWindow;

class MinimizeButton
{
public:
	MinimizeButton(MainWindow *mainWindow);

	bool create(int x, int y, int w, int h);

	int getRecommendedWidth();
	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }


private:
	MainWindow *mainWindow_;
	HWND hwnd_;

	bool bMouseTracking_;
	const int MARGIN = 2;

	static MinimizeButton *this_;
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