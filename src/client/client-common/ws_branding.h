#pragma once

// Wide-string (wchar_t) variants of branding macros defined as compile definitions
// by CMake (via add_compile_definitions in CMakeLists.txt).  Windows APIs frequently
// require wide strings, so this header avoids repeating L"..." conversions everywhere.

#define _WS_WIDEN2(x) L##x
#define _WS_WIDEN(x) _WS_WIDEN2(x)

#define WS_APP_EXECUTABLE_NAME_W _WS_WIDEN(WS_APP_EXECUTABLE_NAME)
#define WS_APP_IDENTIFIER_W _WS_WIDEN(WS_APP_IDENTIFIER)
#define WS_PRODUCT_NAME_W _WS_WIDEN(WS_PRODUCT_NAME)
#define WS_PRODUCT_NAME_LOWER_W _WS_WIDEN(WS_PRODUCT_NAME_LOWER)
#define WS_PRODUCT_NAME_UPPER_W _WS_WIDEN(WS_PRODUCT_NAME_UPPER)
#define WS_WIN_CONFIG_SUBDIR_W _WS_WIDEN(WS_WIN_CONFIG_SUBDIR)
#define WS_WIN_IKEV2_CONNECTION_NAME_W _WS_WIDEN(WS_WIN_IKEV2_CONNECTION_NAME)
#define WS_SETTINGS_ORG_W _WS_WIDEN(WS_SETTINGS_ORG)
#define WS_SETTINGS_APP_W _WS_WIDEN(WS_SETTINGS_APP)
