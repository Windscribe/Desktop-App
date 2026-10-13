#pragma once

#include <Windows.h>

#include <string>

namespace wsl {

// Non-throwing replacement for the deprecated std::wstring_convert, for log text.  Invalid UTF-8 falls back to the
// system ANSI code page, the encoding of localized system error messages; a wrong guess garbles the text but never fails.
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
