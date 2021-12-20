#pragma once

// custom messages
#define WI_CLICK_CLOSE			WM_USER + 1
#define WI_CLICK_MINIMIZE		WM_USER + 2
#define WI_CLICK_SETTINGS		WM_USER + 3
#define WI_CLICK_EULA			WM_USER + 4
#define WI_CLICK_INSTALL		WM_USER + 5
#define WI_CLICK_ESCAPE			WM_USER + 6
#define WI_CLICK_SELECT_PATH	WM_USER + 7
#define WI_FORCE_REDRAW			WM_USER + 8
#define WI_INSTALLER_STATE		WM_USER + 9   // wParam - progress, lParam - current state
#define WI_UPDATE_GIF_REDRAW    WM_USER + 10