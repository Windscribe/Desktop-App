#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>
#include "TextItem.h"

class MainWindow;

class DesktopShortcutControl
{
public:
	DesktopShortcutControl(MainWindow *mainWindow);
	virtual ~DesktopShortcutControl();

	bool create(int x, int y, int w, int h);

	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }
	bool getChecked() const { return isChecked_; }
	void setChecked(bool isChecked);

private:
	MainWindow *mainWindow_;
	HWND hwnd_;
	TextItem *textItem_;

	const int TIMER_ID = 1111;
	const int RIGHT_MARGIN = 12;
	const int BOTTOM_LINE_HEIGHT = 2;
	const int BOTTOM_LINE_LEFT_MARGIN = 12;
	bool bMouseTracking_;

	double buttonPos_; // 0.0 - left(off), 1.0 - right(on)
	bool isChecked_;


	static DesktopShortcutControl *this_;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void fillRoundRectangle(Gdiplus::Graphics* g, Gdiplus::Brush *p, Gdiplus::Rect& rect, UINT8 radius);
	void onPaint();
	void redraw();
	void onMouseMove(int x, int y);
	void onMouseLeave();
	void onLeftButtonUp();
	void onTimer();

	void drawControl(HDC hdc);
	void drawButton(Gdiplus::Graphics *graphics, int x, int y);
	bool isMouseOverButton(int x, int y);
};