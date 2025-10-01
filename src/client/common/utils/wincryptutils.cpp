#include "wincryptutils.h"

#include <windows.h>
#include <dpapi.h>

#include <iomanip>
#include <sstream>
#include <system_error>

#include "wsscopeguard.h"

namespace wsl {

using namespace std;

string WinCryptUtils::encrypt(const wstring &str, EncodeMode mode)
{
    DATA_BLOB dataOut = { 0, nullptr };
    auto cleanup = wsScopeGuard([&] {
        if (dataOut.pbData) {
            ::LocalFree(dataOut.pbData);
        }
    });

    DATA_BLOB dataIn;
    dataIn.cbData = str.size() * sizeof(str.at(0));
    dataIn.pbData = (BYTE*)str.c_str();

    BOOL result = ::CryptProtectData(&dataIn, L"Windscribe Desktop Client", NULL,
                                     NULL, NULL, 0, &dataOut);
    if (result == FALSE) {
        throw system_error(::GetLastError(), generic_category(), "WinCryptUtils::encrypt CryptProtectData failed");
    }

    if (mode == EncodeHex) {
        ostringstream ss;
        for (auto i = 0; i < dataOut.cbData; ++i) {
            ss << hex << setfill('0') << setw(2) << (unsigned short)dataOut.pbData[i];
        }

        return ss.str();
    }

    return string((const char*)dataOut.pbData, dataOut.cbData);
}

wstring WinCryptUtils::decrypt(const std::string &str, EncodeMode mode)
{
    DATA_BLOB dataIn;
    string decoded;

    if (mode == EncodeHex) {
        auto len = str.length();
        if (len % 2 != 0) {
            throw system_error(ERROR_INVALID_DATA, generic_category(), "WinCryptUtils::decrypt passed invalid hex input");
        }

        for (auto i = 0; i < len; i += 2) {
            char chr = (char)(int)strtol(str.substr(i, 2).c_str(), nullptr, 16);
            decoded.push_back(chr);
        }

        dataIn.cbData = decoded.size();
        dataIn.pbData = (BYTE*)decoded.c_str();
    } else {
        dataIn.cbData = str.size();
        dataIn.pbData = (BYTE*)str.c_str();
    }

    DATA_BLOB dataOut = { 0, nullptr };
    auto cleanup = wsScopeGuard([&] {
        if (dataOut.pbData) {
            ::LocalFree(dataOut.pbData);
        }
    });

    BOOL result = ::CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut);
    if (result == FALSE) {
        throw system_error(::GetLastError(), generic_category(), "WinCryptUtils::decrypt CryptUnprotectData failed");
    }

    return wstring((const wchar_t*)dataOut.pbData, dataOut.cbData/sizeof(wchar_t));
}

}
