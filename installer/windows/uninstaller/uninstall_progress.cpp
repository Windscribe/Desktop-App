#include "uninstall_progress.h"

using namespace std;

HWND UninstallProgress::hwndMainWindow;
HWND UninstallProgress::hwndStaticProgress;

UninstallProgress::UninstallProgress()
{

}

UninstallProgress::~UninstallProgress()
{
 if(hwndMainWindow!=nullptr)
  {
   DestroyWindow(hwndMainWindow);
   UnregisterClass(szWindowClass,hInst);
  }


}

bool UninstallProgress::registerClass(const HINSTANCE &hInstance,const int &nCmdShow, const wstring &title, const wstring &class_name)
{
//Registers the window class.

 wcscpy_s(szTitle,title.c_str());
 wcscpy_s(szWindowClass,class_name.c_str());

 WNDCLASSEX wcex;

 wcex.cbSize = sizeof(WNDCLASSEX);

 wcex.style			= CS_HREDRAW | CS_VREDRAW;
 wcex.lpfnWndProc	= WndProc;
 wcex.cbClsExtra		= 0;
 wcex.cbWndExtra		= 0;
 wcex.hInstance		= hInstance;
 wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BBBB));
 wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
 wcex.hbrBackground	= CreateSolidBrush(color_of_a_background); //blue window background
 wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BBBB);
 wcex.lpszClassName	= szWindowClass;
 wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

 RegisterClassEx(&wcex);


// Perform application initialization:
 if (!InitInstance (hInstance, nCmdShow))
 {
  return false;
 }

 return  true;
}
//---------------------------------------------------------------------------

BOOL UninstallProgress::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
// Saves instance handle and creates main window
   hInst = hInstance; // Store instance handle in our global variable

   RECT wr = {0, 0, width_window, height_window};    // set the size, but not the position
   AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);    // adjust the size

  //Window at screen center

   int x = (GetSystemMetrics (SM_CXSCREEN) >> 1) - (width_window  >> 1);
   int y = (GetSystemMetrics (SM_CYSCREEN) >> 1) - (height_window >> 1);

   int width=wr.right - wr.left;
   int height=wr.bottom - wr.top;
   hwndMainWindow = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,x, y, width, height, NULL, NULL, hInstance, NULL);



   if(hwndMainWindow==nullptr)
   {
  //  DWORD ret = GetLastError();

    return false;
   }

   hwndStaticProgress = CreateWindow(
               L"Static",
               L"",
               WS_CHILD |SS_NOTIFY | WS_VISIBLE |SS_CENTER,
               progress_x,
               progress_y,
               progress_width,
               progress_height,
               hwndMainWindow,
               reinterpret_cast<HMENU>(ID_StaticProgress),
               reinterpret_cast<HINSTANCE>(GetWindowLong(hwndMainWindow, GWL_HINSTANCE)),NULL);

   HFONT h_font2;

   h_font2 = CreateFont(26, 0, 0, 0,
                 FW_NORMAL, 0,
       0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                   DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
   SendMessage(hwndStaticProgress, WM_SETFONT, reinterpret_cast<WPARAM>(h_font2), TRUE);



   //ShowWindow(hwndMainWindow, nCmdShow);
   //UpdateWindow(hwndMainWindow);

   return true;
}




LRESULT CALLBACK UninstallProgress::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//Processes messages for the main window.
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return

 switch (message)
   {
    case WM_CREATE:
    {
     break;
    }

    case WM_CTLCOLORBTN:
    {
     HDC hdc = reinterpret_cast<HDC>(wParam);

     SetTextColor(hdc,RGB(0,0,0));
     SetBkMode(hdc,TRANSPARENT);
     return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
    }

    case WM_CTLCOLORSTATIC:
    {
     HDC hdc = reinterpret_cast<HDC>(wParam);

     SetTextColor(hdc, RGB(255,255,255));
     SetBkMode(hdc, TRANSPARENT);
     return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
    }

    case WM_COMMAND:
     int wmId, wmEvent;

        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
    {

    }
        break;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;


    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
 }

 return 0;
}

HWND UninstallProgress::GethHwndMainWindow()
{
 return hwndMainWindow;
}

void UninstallProgress::setProgress(const wstring &text)
{
 std::wstring wstr(text.begin(), text.end());
 setText(hwndMainWindow,ID_StaticProgress, wstr.c_str());
}


int UninstallProgress::setText(const HWND &hwnd,const int &id,const wstring &text)
{
 int ret;

 ret = SetDlgItemText(hwnd, id, text.c_str());
 RECT rect;
 HWND hctrl;
 hctrl = GetDlgItem(hwnd, id);
 GetClientRect(hctrl, &rect);
 MapWindowPoints(hctrl, hwnd,reinterpret_cast<POINT *>(&rect), 2);
 InvalidateRect(hwnd, &rect, true);

 return ret;
}
