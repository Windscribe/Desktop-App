#include "DesktopShortcutControl.h"
#include "Application.h"
#include "DPIScale.h"
#include "TextItem.h"
#include <Windowsx.h>

DesktopShortcutControl *DesktopShortcutControl::this_ = NULL;

using namespace Gdiplus;

DesktopShortcutControl::DesktopShortcutControl(MainWindow *mainWindow) : mainWindow_(mainWindow), bMouseTracking_(false),
	buttonPos_(1.0), isChecked_(true)
{
	this_ = this;
	textItem_ = new TextItem(L"Desktop shortcut", 12 * SCALE_FACTOR, true);
}

DesktopShortcutControl::~DesktopShortcutControl()
{
	delete textItem_;
}

bool DesktopShortcutControl::create(int x, int y, int w, int h)
{
	TCHAR szWindowClass[] = L"WindscribeInstallerDesktopShortcutControl_WC2";

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g_application->getInstance();
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = NULL;

	if (RegisterClassEx(&wcex) == NULL)
	{
		return false;
	}

	hwnd_ = CreateWindowEx(WS_EX_COMPOSITED, szWindowClass, NULL, WS_CHILD | WS_CLIPCHILDREN,
		x, y, w, h, mainWindow_->getHwnd(), NULL, g_application->getInstance(), NULL);
	DWORD dw = GetLastError();
	if (!hwnd_)
	{
		return false;
	}

	return true;
}

int DesktopShortcutControl::getRecommendedHeight()
{
	return (int)(50 * SCALE_FACTOR);
}

LRESULT CALLBACK DesktopShortcutControl::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return this_->realWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK DesktopShortcutControl::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return (LRESULT)GetStockObject(NULL_BRUSH);
	case WM_PAINT:
		onPaint();
		break;
	case WI_FORCE_REDRAW:
		redraw();
		return 0;
	case WM_MOUSEMOVE:
		onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_MOUSELEAVE:
		onMouseLeave();
		break;

	case WM_LBUTTONUP:
		onLeftButtonUp();
		break;

	case WM_SETCURSOR:
		if (bMouseTracking_)
		{
			SetCursor(LoadCursor(nullptr, IDC_HAND));
			return TRUE;
		}
		else
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

	case WM_TIMER:
		if (wParam == TIMER_ID)
		{
			onTimer();
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	};
	return 0;
}

void DesktopShortcutControl::fillRoundRectangle(Graphics* g, Brush *p, Rect& rect, UINT8 radius)
{
	GraphicsPath path;
	path.AddLine(rect.X + radius, rect.Y, rect.X + rect.Width - (radius * 2), rect.Y);
	path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y, radius * 2, radius * 2, 270, 90);
	path.AddLine(rect.X + rect.Width, rect.Y + radius, rect.X + rect.Width, rect.Y + rect.Height - (radius * 2));
	path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 0, 90);
	path.AddLine(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height, rect.X + radius, rect.Y + rect.Height);
	path.AddArc(rect.X, rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 90, 90);
	path.AddLine(rect.X, rect.Y + rect.Height - (radius * 2), rect.X, rect.Y + radius);
	path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
	path.CloseFigure();

	g->FillPath(p, &path);
}

void DesktopShortcutControl::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd_, &ps);

	drawControl(hdc);
	EndPaint(hwnd_, &ps);
}

void DesktopShortcutControl::redraw()
{
	HDC hdc = GetDC(hwnd_);
	drawControl(hdc);
	ReleaseDC(hwnd_, hdc);
}

void DesktopShortcutControl::setChecked(bool isChecked)
{
	isChecked_ = isChecked;
	if (!isChecked_)
	{
		buttonPos_ = 0.0;
	}
	else
	{
		buttonPos_ = 1.0f;
	}
	redraw();
}

void DesktopShortcutControl::onMouseMove(int x, int y)
{
	if (!bMouseTracking_)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = HOVER_DEFAULT;
		tme.hwndTrack = hwnd_;
		TrackMouseEvent(&tme);

		if (isMouseOverButton(x, y))
		{
			bMouseTracking_ = true;
			SetCursor(LoadCursor(nullptr, IDC_HAND));
			redraw();
		}
	}
	else
	{
		if (!isMouseOverButton(x, y))
		{
			bMouseTracking_ = false;
			redraw();
		}
	}
}

void DesktopShortcutControl::onMouseLeave()
{
	bMouseTracking_ = false;
	redraw();
}

void DesktopShortcutControl::onLeftButtonUp()
{
	if (bMouseTracking_)
	{
		isChecked_ = !isChecked_;
		SetTimer(hwnd_, TIMER_ID, 1, NULL);
	}
}

void DesktopShortcutControl::onTimer()
{
	const double step = 0.15;
	if (!isChecked_)
	{
		buttonPos_ -= step;
		if (buttonPos_ < 0.0)
		{
			buttonPos_ = 0.0;
			KillTimer(hwnd_, TIMER_ID);
		}
	}
	else
	{
		buttonPos_ += step;
		if (buttonPos_ > 1.0f)
		{
			buttonPos_ = 1.0f;
			KillTimer(hwnd_, TIMER_ID);
		}
	}
	redraw();
}

void DesktopShortcutControl::drawControl(HDC hdc)
{
	RECT rc;
	GetClientRect(hwnd_, &rc);

	Bitmap doubleBufferBmp(rc.right, rc.bottom);
	Graphics *graphics = Graphics::FromImage(&doubleBufferBmp);
	graphics->SetSmoothingMode(SmoothingModeHighQuality);

	// draw main window background image
	RECT winRect;
	GetWindowRect(hwnd_, &winRect);
	POINT p1 = { winRect.left, winRect.top };
	POINT p2 = { winRect.right, winRect.bottom };
	ScreenToClient(mainWindow_->getHwnd(), &p1);
	ScreenToClient(mainWindow_->getHwnd(), &p2);

	mainWindow_->drawBackground(graphics, p1.x, p1.y, p2.x - p1.x, p2.y - p1.y);

	// draw title
	Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255));
	Gdiplus::StringFormat strformat;
	strformat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	graphics->DrawString(textItem_->text(), wcslen(textItem_->text()), textItem_->getFont(),
		Gdiplus::RectF(0, 0, rc.right, rc.bottom), &strformat, &brush);

	// draw button
	drawButton(graphics, rc.right - g_application->getImageResources()->getWhiteToggleBg()->GetWidth() - RIGHT_MARGIN * SCALE_FACTOR,
		(rc.bottom - BOTTOM_LINE_HEIGHT * SCALE_FACTOR - g_application->getImageResources()->getWhiteToggleBg()->GetHeight()) / 2);


	// draw bottom line
	SolidBrush brushBottomLine(Gdiplus::Color(70, 255, 255, 255));
	graphics->FillRectangle(&brushBottomLine, REAL(BOTTOM_LINE_LEFT_MARGIN * SCALE_FACTOR), rc.bottom - BOTTOM_LINE_HEIGHT * SCALE_FACTOR - 1, REAL(rc.right - BOTTOM_LINE_LEFT_MARGIN * SCALE_FACTOR), BOTTOM_LINE_HEIGHT * SCALE_FACTOR);


	delete graphics;

	// draw on screen
	{
		Graphics screenGraphics(hdc);
		screenGraphics.DrawImage(&doubleBufferBmp, 0, 0);
	}
}

void DesktopShortcutControl::drawButton(Graphics *graphics, int x, int y)
{
	int w = g_application->getImageResources()->getWhiteToggleBg()->GetWidth();
	int h = g_application->getImageResources()->getWhiteToggleBg()->GetHeight();

	graphics->DrawImage(g_application->getImageResources()->getWhiteToggleBg(), REAL(x), REAL(y));

	Gdiplus::ColorMatrix matrix =
	{
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 0, static_cast<float>(buttonPos_), 0,
		0, 0, 0, 0, 1
	};
	Gdiplus::ImageAttributes attrib;
	attrib.SetColorMatrix(&matrix);
	graphics->DrawImage(g_application->getImageResources()->getGreenToggleBg(), Gdiplus::Rect(x, y, w, h), 0, 0, w, h, Gdiplus::UnitPixel, &attrib);

	int offs = (h - g_application->getImageResources()->getToggleButtonBlack()->GetHeight()) / 2;
	int offsEnd = w - offs - g_application->getImageResources()->getToggleButtonBlack()->GetWidth();
	int addOffs = (offsEnd - offs) * buttonPos_;
	graphics->DrawImage(g_application->getImageResources()->getToggleButtonBlack(), x + offs + addOffs, y + offs);
}

bool DesktopShortcutControl::isMouseOverButton(int x, int y)
{
	RECT rc;
	GetClientRect(hwnd_, &rc);
	RECT btnRect;
	int imgWidth = g_application->getImageResources()->getWhiteToggleBg()->GetWidth();
	int imgHeight = g_application->getImageResources()->getWhiteToggleBg()->GetHeight();

	const int MARGIN = 5 * SCALE_FACTOR;
	btnRect.left = rc.right - g_application->getImageResources()->getWhiteToggleBg()->GetWidth() - RIGHT_MARGIN * SCALE_FACTOR;
	btnRect.top = (rc.bottom - BOTTOM_LINE_HEIGHT * SCALE_FACTOR - g_application->getImageResources()->getWhiteToggleBg()->GetHeight()) / 2;
	btnRect.right = btnRect.left + imgWidth;
	btnRect.bottom = btnRect.top + imgHeight;

	POINT pt = { x, y };
	return PtInRect(&btnRect, pt);
}
