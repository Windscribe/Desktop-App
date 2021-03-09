#include "MinimizeButton.h"
#include "Application.h"
#include "DPIScale.h"

MinimizeButton *MinimizeButton::this_ = NULL;

using namespace Gdiplus;

MinimizeButton::MinimizeButton(MainWindow *mainWindow) : mainWindow_(mainWindow), bMouseTracking_(false)
{
	this_ = this;
}

bool MinimizeButton::create(int x, int y, int w, int h)
{
	TCHAR szWindowClass[] = L"WindscribeMinimizeButton_WC2";

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g_application->getInstance();
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_HAND);
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = NULL;

	if (RegisterClassEx(&wcex) == NULL)
	{
		return false;
	}

	hwnd_ = CreateWindowEx(WS_EX_COMPOSITED, szWindowClass, szWindowClass, WS_CHILD | WS_VISIBLE,
		x, y, w, h, mainWindow_->getHwnd(), nullptr, g_application->getInstance(), nullptr);
	DWORD dw = GetLastError();
	if (!hwnd_)
	{
		return false;
	}

	return true;
}

int MinimizeButton::getRecommendedWidth()
{
	return (int)(g_application->getImageResources()->getMinimizeIcon()->GetWidth() + MARGIN * SCALE_FACTOR * 2);
}

int MinimizeButton::getRecommendedHeight()
{
	return (int)(g_application->getImageResources()->getMinimizeIcon()->GetHeight() + MARGIN * SCALE_FACTOR * 2);
}

LRESULT CALLBACK MinimizeButton::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return this_->realWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MinimizeButton::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return (LRESULT)GetStockObject(NULL_BRUSH);
	case WM_PAINT:
		onPaint();
		break;
	case WM_MOUSEMOVE:
		onMouseMove();
		break;

	case WM_LBUTTONDOWN:
		onLeftButtonDown();
		break;
	case WM_LBUTTONUP:
		onLeftButtonUp();
		break;

	case WM_MOUSEHOVER:
		onMouseEnter();
		break;
	case WM_MOUSELEAVE:
		onMouseLeave();
		break;

	case WI_FORCE_REDRAW:
		redraw();
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	};
	return 0;
}

void MinimizeButton::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd_, &ps);
	drawButton(hdc);
	EndPaint(hwnd_, &ps);
}

void MinimizeButton::redraw()
{
	HDC hdc = GetDC(hwnd_);
	drawButton(hdc);
	ReleaseDC(hwnd_, hdc);
}

void MinimizeButton::onMouseMove()
{
	if (!bMouseTracking_)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = HOVER_DEFAULT;
		tme.hwndTrack = hwnd_;
		TrackMouseEvent(&tme);

		bMouseTracking_ = true;
		redraw();
	}
}
void MinimizeButton::onMouseEnter()
{
}
void MinimizeButton::onMouseLeave()
{
	bMouseTracking_ = false;
	redraw();
}
void MinimizeButton::onLeftButtonDown()
{
}

void MinimizeButton::onLeftButtonUp()
{
	SendMessage(mainWindow_->getHwnd(), WI_CLICK_MINIMIZE, 0, 0);
}
void MinimizeButton::drawButton(HDC hdc)
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

	// draw image with opacity
	float alpha = bMouseTracking_ ? 1.0f : 0.5f;
	ColorMatrix matrix =
	{
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 0, alpha, 0,
		0, 0, 0, 0, 1
	};
	ImageAttributes attrib;
	attrib.SetColorMatrix(&matrix);

	int imageWidth = g_application->getImageResources()->getMinimizeIcon()->GetWidth();
	int imageHeight = g_application->getImageResources()->getMinimizeIcon()->GetHeight();
	graphics->DrawImage(g_application->getImageResources()->getMinimizeIcon(),
		Rect((rc.right - imageWidth) / 2, (rc.bottom - imageHeight) / 2, imageWidth, imageHeight),
		0, 0, imageWidth, imageHeight, UnitPixel, &attrib);
	
	delete graphics;

	// draw on screen
	{
		Graphics screenGraphics(hdc);
		screenGraphics.DrawImage(&doubleBufferBmp, 0, 0);
	}
}