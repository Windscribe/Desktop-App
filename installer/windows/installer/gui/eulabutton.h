#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class MainWindow;
class TextItem;

class EulaButton
{
public:
	EulaButton(MainWindow *mainWindow, const wchar_t *szTitle);
	virtual ~EulaButton();

	bool create(int x, int y, int w, int h);

	int getRecommendedWidth();
	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }


private:
	MainWindow *mainWindow_;
	HWND hwnd_;
	TextItem *titleItem_;

	bool bMouseTracking_;
	const int MARGIN = 1;

	static EulaButton *this_;
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