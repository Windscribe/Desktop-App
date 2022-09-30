#include "InstallButton.h"
#include "Application.h"
#include "DPIScale.h"
#include "TextItem.h"

InstallButton *InstallButton::this_ = NULL;

using namespace Gdiplus;

InstallButton::InstallButton(MainWindow *mainWindow, const wchar_t *szTitle) : mainWindow_(mainWindow), bMouseTracking_(false),
	state_(INSTALL_TITLE), progress_(0)

{
	this_ = this;
	titleItem_ = new TextItem(szTitle, (int)(13 * SCALE_FACTOR), false);
}

InstallButton::~InstallButton()
{
	delete titleItem_;
}

bool InstallButton::create(int x, int y, int w, int h)
{
	TCHAR szWindowClass[] = L"WindscribeInstallerButton_WC2";

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

	hwnd_ = CreateWindowEx(WS_EX_COMPOSITED, szWindowClass, titleItem_->text(), WS_CHILD | WS_VISIBLE,
		x, y, w, h, mainWindow_->getHwnd(), nullptr, g_application->getInstance(), nullptr);
	DWORD dw = GetLastError();
	if (!hwnd_)
	{
		return false;
	}

	return true;
}

int InstallButton::getRecommendedWidth()
{
	return titleItem_->textWidth() + g_application->getImageResources()->getForwardArrow()->GetWidth() + (int)(TEXT_MARGIN * SCALE_FACTOR * 2) +
		+(int)(BEFORE_ARROW_MARGIN * SCALE_FACTOR);
}

int InstallButton::getRecommendedHeight()
{
	return (int)(32 * SCALE_FACTOR);
}

void InstallButton::setState(BUTTON_STATE state)
{
	state_ = state;
	progress_ = 0;
	if (state_ == WAIT_WITH_PROGRESS || state_ == WAIT_WITHOUT_PROGRESS)
	{
		SetTimer(hwnd_, TIMER_ID, 1, NULL);
	}
	else
	{
		KillTimer(hwnd_, TIMER_ID);
	}

	if (state_ == INSTALL_TITLE)
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
	}
	else
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	redraw();
}

void InstallButton::setProgress(int progress)
{
	progress_ = progress;
	redraw();
}

LRESULT CALLBACK InstallButton::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return this_->realWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK InstallButton::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	case WM_MOUSEHOVER:
		onMouseEnter();
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
		if (state_ == INSTALL_TITLE)
		{
			SetCursor(LoadCursor(NULL, IDC_HAND));
		}
		else
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
		return TRUE;


	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	};
	return 0;
}

void InstallButton::fillRoundRectangle(Graphics* g, Brush *p, Rect& rect, UINT8 radius)
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

void InstallButton::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd_, &ps);
	drawButton(hdc);
	EndPaint(hwnd_, &ps);
}

void InstallButton::redraw()
{
	HDC hdc = GetDC(hwnd_);
	drawButton(hdc);
	ReleaseDC(hwnd_, hdc);
}

void InstallButton::onMouseMove()
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
void InstallButton::onMouseEnter()
{
	int g = 0;
}
void InstallButton::onMouseLeave()
{
	bMouseTracking_ = false;
	redraw();
}

void InstallButton::onLeftButtonUp()
{
	if (state_ == INSTALL_TITLE)
	{
		SendMessage(mainWindow_->getHwnd(), WI_CLICK_INSTALL, 0, 0);
	}
}

void InstallButton::onTimer()
{
	curRotation_ += 6;
	if (curRotation_ > 360)
	{
		curRotation_ = 0;
	}
	redraw();
}

void InstallButton::drawButton(HDC hdc)
{
	RECT rc;
	GetClientRect(hwnd_, &rc);

	Bitmap doubleBufferBmp(rc.right, rc.bottom);
	Graphics *graphics = Graphics::FromImage(&doubleBufferBmp);
	graphics->SetSmoothingMode(SmoothingModeHighQuality);
	//graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);
	//graphics->SetPixelOffsetMode(PixelOffsetModeNone);

	// draw main window background image
	RECT winRect;
	GetWindowRect(hwnd_, &winRect);
	POINT p1 = { winRect.left, winRect.top };
	POINT p2 = { winRect.right, winRect.bottom };
	ScreenToClient(mainWindow_->getHwnd(), &p1);
	ScreenToClient(mainWindow_->getHwnd(), &p2);

	mainWindow_->drawBackground(graphics, p1.x, p1.y, p2.x - p1.x, p2.y - p1.y);

	if (state_ == INSTALL_TITLE)
	{
		drawStateInstallTitle(graphics, rc);
	}
	else if (state_ == WAIT_WITH_PROGRESS)
	{
		drawStateWithProgress(graphics, rc);
	}
	else
	{
		drawStateInstallTitle(graphics, rc);
	}

	delete graphics;

	// draw on screen
	{
		Graphics screenGraphics(hdc);
		screenGraphics.DrawImage(&doubleBufferBmp, 0, 0);
	}
}

void InstallButton::drawStateInstallTitle(Gdiplus::Graphics *graphics, const RECT &rc)
{
	SolidBrush brush(bMouseTracking_ ? Color(255, 255, 255) : Color(0x55, 0xFF, 0x8A));
	fillRoundRectangle(graphics, &brush, Rect(0, 0, rc.right - 1, rc.bottom - 1), 16 * SCALE_FACTOR);

	// draw text
	StringFormat format;
	format.SetLineAlignment(StringAlignmentCenter);
	SolidBrush blackBrush(Color(255, 3, 9, 28));
	graphics->DrawString(titleItem_->text(), static_cast<int>(wcslen(titleItem_->text())), titleItem_->getFont(),
                         RectF((REAL)(TEXT_MARGIN * SCALE_FACTOR), 0.0f, (REAL)rc.right, (REAL)rc.bottom), &format, &blackBrush);

	// draw arrow image with 50% opacity
	float alpha = 1.0f;
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

	int imageWidth = g_application->getImageResources()->getForwardArrow()->GetWidth();
	int imageHeight = g_application->getImageResources()->getForwardArrow()->GetHeight();
	graphics->DrawImage(g_application->getImageResources()->getForwardArrow(),
		Rect(titleItem_->textWidth() + (int)(TEXT_MARGIN * SCALE_FACTOR) + (int)(BEFORE_ARROW_MARGIN * SCALE_FACTOR), (rc.bottom - imageHeight) / 2, imageWidth, imageHeight),
		0, 0, imageWidth, imageHeight, UnitPixel, &attrib);
}

void InstallButton::drawStateWithProgress(Gdiplus::Graphics *graphics, const RECT &rc)
{
	SolidBrush brush(Color(255, 255, 255));
	fillRoundRectangle(graphics, &brush, Rect(0, 0, rc.right - 1, rc.bottom - 1), 16 * SCALE_FACTOR);

	// draw text
	StringFormat format;
	format.SetLineAlignment(StringAlignmentCenter);
	format.SetAlignment(StringAlignmentCenter);
	SolidBrush blackBrush(Color(255, 3, 9, 28));

	std::wstring str = std::to_wstring(progress_) + L"%";

	graphics->DrawString(str.c_str(), static_cast<int>(str.length()), g_application->getFontResources()->getFont((int)(12 * SCALE_FACTOR), false),
                         RectF(4 * SCALE_FACTOR, 0.0f, (REAL)rc.right - TEXT_MARGIN * SCALE_FACTOR - g_application->getImageResources()->getForwardArrow()->GetWidth(),
                         (REAL)rc.bottom), &format, &blackBrush);
	/*{
	float alpha = 1.0f;
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

	int imageWidth = g_application->getImageResources()->getForwardArrow()->GetWidth();
	int imageHeight = g_application->getImageResources()->getForwardArrow()->GetHeight();
	graphics->DrawImage(g_application->getImageResources()->getForwardArrow(),
		Rect(titleItem_->textWidth() + (int)(TEXT_MARGIN * SCALE_FACTOR) + (int)(BEFORE_ARROW_MARGIN * SCALE_FACTOR), (rc.bottom - imageHeight) / 2, imageWidth, imageHeight),
		0, 0, imageWidth, imageHeight, UnitPixel, &attrib);
	}*/
	// draw spinner icon
	int imageWidth = 16 * SCALE_FACTOR;// g_application->getImageResources()->getForwardArrow()->GetWidth();
	int imageHeight = 16 * SCALE_FACTOR;// g_application->getImageResources()->getForwardArrow()->GetHeight();

	Rect rrr(rc.right - (int)(TEXT_MARGIN * SCALE_FACTOR) - imageWidth + 5 * SCALE_FACTOR, (rc.bottom - imageHeight) / 2,
		imageWidth, imageHeight);
	Pen pen(Color(167, 170, 176), 2.0 * SCALE_FACTOR);
	graphics->DrawArc(&pen, rrr, curRotation_, 270);
}