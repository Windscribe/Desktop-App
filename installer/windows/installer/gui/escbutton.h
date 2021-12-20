#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>
#include "TextItem.h"

class MainWindow;

class EscButton
{
public:
	EscButton(MainWindow *mainWindow);
	virtual ~EscButton();

	bool create(int x, int y, int w, int h);

	int getRecommendedWidth();
	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }


private:
	MainWindow *mainWindow_;
	HWND hwnd_;
	TextItem *textItem_;

	bool bMouseTracking_;
	const int MARGIN = 2;

	static EscButton *this_;
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