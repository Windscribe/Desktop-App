#ifndef Uninstall_Progress_H
#define Uninstall_Progress_H

#include <Windows.h>
#include <string>
#include "../Utils/window_parameters.h"


class UninstallProgress
{
 private:
    #define IDM_EXIT				105
    #define IDI_BBBB			107
    #define IDI_SMALL				108
    #define IDC_BBBB			109
    #define MAX_LOADSTRING 100


    #define ID_StaticProgress 6


    #ifndef GWL_HINSTANCE
    #define GWL_HINSTANCE (-6)
    #endif


  HINSTANCE hInst;						//current instance
  TCHAR szTitle[MAX_LOADSTRING];		//The title bar text
  TCHAR szWindowClass[MAX_LOADSTRING];	//the main window class name


    static HWND hwndMainWindow;
  static HWND hwndStaticProgress;



  static int setText(const HWND &hwnd,const int &id,const std::wstring &text);

  BOOL	 InitInstance(HINSTANCE, int);
  static LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


public:
  HWND GethHwndMainWindow();
  void setProgress(const std::wstring &text);
  bool registerClass(const HINSTANCE &hInstance,const int &nCmdShow, const std::wstring &title, const std::wstring &class_name);
  UninstallProgress();
  ~UninstallProgress();
};

#endif // Uninstall_Progress_H
