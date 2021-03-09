#include "EulaButton.h"
#include "Application.h"
#include "DPIScale.h"

EulaButton *EulaButton::this_ = NULL;

using namespace Gdiplus;

EulaButton::EulaButton(MainWindow *mainWindow, const wchar_t *szTitle) : mainWindow_(mainWindow), bMouseTracking_(false)
{
	this_ = this;
	titleItem_ = new TextItem(szTitle, (int)(13 * SCALE_FACTOR), false);
}
EulaButton::~EulaButton()
{
	delete titleItem_;
}

bool EulaButton::create(int x, int y, int w, int h)
{
	TCHAR szWindowClass[] = L"WindscribeEulaButton_WC2";

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

int EulaButton::getRecommendedWidth()
{
	return (int)(titleItem_->textWidth() + MARGIN * SCALE_FACTOR * 2);
}

int EulaButton::getRecommendedHeight()
{
	return (int)(titleItem_->textHeight() + MARGIN * SCALE_FACTOR * 2);
}

LRESULT CALLBACK EulaButton::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return this_->realWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK EulaButton::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	};
	return 0;
}

void EulaButton::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd_, &ps);
	drawButton(hdc);
	EndPaint(hwnd_, &ps);
}

void EulaButton::redraw()
{
	HDC hdc = GetDC(hwnd_);
	drawButton(hdc);
	ReleaseDC(hwnd_, hdc);
}

void EulaButton::onMouseMove()
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
void EulaButton::onMouseEnter()
{
}
void EulaButton::onMouseLeave()
{
	bMouseTracking_ = false;
	redraw();
}
void EulaButton::onLeftButtonDown()
{
}

void EulaButton::onLeftButtonUp()
{
	SendMessage(mainWindow_->getHwnd(), WI_CLICK_EULA, 0, 0);
}
void EulaButton::drawButton(HDC hdc)
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

	//Gdiplus::SolidBrush brush2(Gdiplus::Color(255, 92, 28));
	//graphics->FillRectangle(&brush2, 0, 0, rc.right, rc.bottom);
	
	StringFormat format;
	format.SetLineAlignment(StringAlignmentCenter);
	format.SetAlignment(StringAlignmentCenter);
	SolidBrush brush(Color(bMouseTracking_ ? 204 : 128, 255, 255, 255));
	graphics->DrawString(titleItem_->text(), wcslen(titleItem_->text()), titleItem_->getFont(), RectF(0.0f, 0.0f, (REAL)rc.right, (REAL)rc.bottom), &format, &brush);

	delete graphics;

	// draw on screen
	{
		Graphics screenGraphics(hdc);
		screenGraphics.DrawImage(&doubleBufferBmp, 0, 0);
	}
}