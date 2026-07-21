#pragma once

#include <Windows.h>

#include <string>

namespace wsl {

// Non-throwing replacement for the deprecated std::wstring_convert, which throws
// std::range_error on invalid UTF-8.  That matters here because some converted strings
// are raw console output of child processes (e.g. 7zr.exe), which on localized systems
// may not be UTF-8.  A logging helper must never take the process down.
//
// Tries UTF-8 first; on invalid input falls back to the system ANSI code page (the most
// likely encoding of non-UTF-8 child process output); a wrong guess garbles the text but
// never fails.
inline std::wstring toWideString(const std::string &str)
{
    if (str.empty()) {
        return std::wstring();
    }

    const int size = static_cast<int>(str.size());
    UINT codePage = CP_UTF8;
    int len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), size, nullptr, 0);
    if (len <= 0) {
        codePage = CP_ACP;
        len = ::MultiByteToWideChar(CP_ACP, 0, str.data(), size, nullptr, 0);
        if (len <= 0) {
            return L"<string conversion failed>";
        }
    }

    std::wstring result(len, L'\0');
    ::MultiByteToWideChar(codePage, 0, str.data(), size, result.data(), len);
    return result;
}

}
