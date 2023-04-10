#pragma once

#include <string>

namespace wsl {

class WinCryptUtils
{
public:
    enum EncodeMode {
        EncodeNone,
        EncodeHex
    };

    // Encrypts the given Unicode string and encodes the result as specified.  The result
    // can only be decrypted by the same Windows user account.  Additionally, the user
    // must be on the same PC, unless they are using a roaming profile.  If EncodeNone
    // is specified, treat the result string as an array of bytes.  Passing an empty
    // string is valid and will produce an encrypted result.
    // Throws a system_error object on failure.
    static std::string encrypt(const std::wstring &str, EncodeMode mode = EncodeNone);

    // Decodes the given string, if specified, then decrypts the result.  The decrypted byte
    // array is then returned as a Unicode string.  The input string can only be decrypted by
    // the same Windows user account that encrypted it.  Additionally, the user must be on
    // the same PC, unless they are using a roaming profile.
    // Throws a system_error object on failure.
    static std::wstring decrypt(const std::string &str, EncodeMode mode = EncodeNone);
};

}
