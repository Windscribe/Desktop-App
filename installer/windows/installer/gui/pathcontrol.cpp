#include "PathControl.h"
#include "Application.h"
#include "DPIScale.h"
#include "TextItem.h"
#include <Windowsx.h>

PathControl *PathControl::this_ = NULL;

using namespace Gdiplus;

PathControl::PathControl(MainWindow *mainWindow) : mainWindow_(mainWindow), bMouseTracking_(false)
{
	this_ = this;
}

PathControl::~PathControl()
{
}

bool PathControl::create(int x, int y, int w, int h, const std::wstring &path)
{
	TCHAR szWindowClass[] = L"WindscribeInstallerPathControl_WC2";

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

	int editBoxWidth = w - g_application->getImageResources()->getFolderIcon()->GetWidth() - 2 * RIGHT_MARGIN * SCALE_FACTOR;
	int editBoxHeight = (12 + 4) * SCALE_FACTOR;
	int editBoxX = 0;
	int editBoxY = (h - editBoxHeight - BOTTOM_LINE_HEIGHT * SCALE_FACTOR) / 2;
	hwndEdit_ = CreateWindowEx(WS_EX_TRANSPARENT, L"Edit", path.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP, 
							editBoxX, editBoxY, editBoxWidth, editBoxHeight, hwnd_,
							(HMENU)IDC_EDIT_CONTROL, g_application->getInstance(), NULL);
	if (!hwndEdit_)
	{
		return false;
	}

	SendMessage(hwndEdit_, WM_SETFONT, (WPARAM)g_application->getFontResources()->getHFont((int)(12 * SCALE_FACTOR), false), MAKELPARAM(FALSE, 0));

	return true;
}

void PathControl::setPath(const std::wstring &path)
{
	SetWindowText(hwndEdit_, path.c_str());
}

std::wstring PathControl::path() const
{
	wchar_t path[MAX_PATH];
	GetWindowText(hwndEdit_, path, MAX_PATH);
	return path;
}

int PathControl::getRecommendedHeight()
{
	return (int)(50 * SCALE_FACTOR);
}

LRESULT CALLBACK PathControl::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return this_->realWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PathControl::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return (LRESULT)GetStockObject(NULL_BRUSH);
	case WM_PAINT:
		onPaint();
		break;
	case WM_MOUSEMOVE:
		onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_MOUSELEAVE:
		onMouseLeave();
		break;

	case WM_LBUTTONUP:
		onLeftButtonUp();
		break;

	case WM_TIMER:
		onTimer();
		break;

	case WI_FORCE_REDRAW:
		redraw();
		return 0;

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
	case WM_CTLCOLOREDIT:
	{
		// we don't want that background to be drawn
		SetBkMode((HDC)wParam, TRANSPARENT);
		SetTextColor((HDC)wParam, RGB(123, 129, 139));
		return (LRESULT)GetStockObject(HOLLOW_BRUSH);
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_EDIT_CONTROL)
		{
			RedrawWindow((HWND)lParam, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	};
	return 0;
}

void PathControl::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd_, &ps);

	drawControl(hdc);
	EndPaint(hwnd_, &ps);
}

void PathControl::redraw()
{
	HDC hdc = GetDC(hwnd_);
	drawControl(hdc);
	ReleaseDC(hwnd_, hdc);
}

void PathControl::onMouseMove(int x, int y)
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

void PathControl::onMouseLeave()
{
	bMouseTracking_ = false;
	redraw();
}

void PathControl::onLeftButtonUp()
{
	if (bMouseTracking_)
	{
		SendMessage(mainWindow_->getHwnd(), WI_CLICK_SELECT_PATH, 0, 0);
	}
}

void PathControl::onTimer()
{
	redraw();
}

void PathControl::drawControl(HDC hdc)
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

	//SolidBrush brush(Gdiplus::Color(255, 0, 28));
	//graphics->FillRectangle(&brush, 0, 0, rc.right, rc.bottom);


	int imgWidth = g_application->getImageResources()->getFolderIcon()->GetWidth();
	int imgHeight = g_application->getImageResources()->getFolderIcon()->GetHeight();

	float alpha = bMouseTracking_ ? 0.8 : 0.4;
	Gdiplus::ColorMatrix matrix =
	{
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 0, alpha, 0,
		0, 0, 0, 0, 1
	};
	Gdiplus::ImageAttributes attrib;
	attrib.SetColorMatrix(&matrix);
	graphics->DrawImage(g_application->getImageResources()->getFolderIcon(),
						Rect(REAL(rc.right - imgWidth - RIGHT_MARGIN * SCALE_FACTOR),
						(rc.bottom - BOTTOM_LINE_HEIGHT * SCALE_FACTOR - imgHeight) / 2, imgWidth, imgHeight),
						0, 0, imgWidth, imgHeight,
						UnitPixel, &attrib);

	SolidBrush brushBottomLine(Gdiplus::Color(70, 255, 255, 255));
	graphics->FillRectangle(&brushBottomLine, REAL(BOTTOM_LINE_LEFT_MARGIN * SCALE_FACTOR), rc.bottom - BOTTOM_LINE_HEIGHT * SCALE_FACTOR - 1, REAL(rc.right - BOTTOM_LINE_LEFT_MARGIN * SCALE_FACTOR), BOTTOM_LINE_HEIGHT * SCALE_FACTOR);


	delete graphics;

	// draw on screen
	{
		Graphics screenGraphics(hdc);
		screenGraphics.DrawImage(&doubleBufferBmp, 0, 0);
	}
}

bool PathControl::isMouseOverButton(int x, int y)
{
	RECT rc;
	GetClientRect(hwnd_, &rc);
	RECT btnRect;
	int imgWidth = g_application->getImageResources()->getFolderIcon()->GetWidth();
	int imgHeight = g_application->getImageResources()->getFolderIcon()->GetHeight();

	const int MARGIN = 5 * SCALE_FACTOR;
	btnRect.left = rc.right - imgWidth - RIGHT_MARGIN * SCALE_FACTOR - MARGIN;
	btnRect.top = (rc.bottom - BOTTOM_LINE_HEIGHT * SCALE_FACTOR - imgHeight) / 2 - MARGIN;
	btnRect.right = btnRect.left + imgWidth + 2 * MARGIN;
	btnRect.bottom = btnRect.top + imgHeight + 2 * MARGIN;

	POINT pt = { x, y };
	return PtInRect(&btnRect, pt);
}