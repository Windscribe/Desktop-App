#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class MainWindow;
class TextItem;

class InstallButton
{
public:
	enum BUTTON_STATE { INSTALL_TITLE, WAIT_WITH_PROGRESS, WAIT_WITHOUT_PROGRESS };

	InstallButton(MainWindow *mainWindow, const wchar_t *szTitle);
	virtual ~InstallButton();

	bool create(int x, int y, int w, int h);

	int getRecommendedWidth();
	int getRecommendedHeight();

	HWND getHwnd() { return hwnd_;  }
	
	void setState(BUTTON_STATE state);
	void setProgress(int progress);


private:
	MainWindow *mainWindow_;
	HWND hwnd_;
	TextItem *titleItem_;
	const int TEXT_MARGIN = 14;
	const int BEFORE_ARROW_MARGIN = 10;
	const int TIMER_ID = 1111;
	bool bMouseTracking_;

	BUTTON_STATE state_;
	int progress_;
	int curRotation_;


	static InstallButton *this_;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void fillRoundRectangle(Gdiplus::Graphics* g, Gdiplus::Brush *p, Gdiplus::Rect& rect, UINT8 radius);
	void onPaint();
	void redraw();
	void onMouseMove();
	void onMouseEnter();
	void onMouseLeave();
	void onLeftButtonUp();
	void onTimer();

	void drawButton(HDC hdc);

	void drawStateInstallTitle(Gdiplus::Graphics *graphics, const RECT &rc);
	void drawStateWithProgress(Gdiplus::Graphics *graphics, const RECT &rc);
};