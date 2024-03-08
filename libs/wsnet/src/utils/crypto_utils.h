#pragma once

#include <string>

namespace crypto_utils
{
    std::string sha1(const std::string &str);
    std::string md5(const std::string &str);
}
