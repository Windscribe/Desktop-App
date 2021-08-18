#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class MainWindow;

class PathControl
{
public:
	PathControl(MainWindow *mainWindow);
	virtual ~PathControl();

	bool create(int x, int y, int w, int h, const std::wstring &path);

	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }

	void setPath(const std::wstring &path);
	std::wstring path() const;

private:
	MainWindow *mainWindow_;
	HWND hwnd_;
	HWND hwndEdit_;

	const int TIMER_ID = 1111;
	const int IDC_EDIT_CONTROL = 100;
	const int RIGHT_MARGIN = 12;
	const int BOTTOM_LINE_HEIGHT = 2;
	const int BOTTOM_LINE_LEFT_MARGIN = 12;
	bool bMouseTracking_;

	static PathControl *this_;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void onPaint();
	void redraw();
	void onMouseMove(int x, int y);
	void onMouseLeave();
	void onLeftButtonUp();
	void onTimer();

	void drawControl(HDC hdc);
	bool isMouseOverButton(int x, int y);
};