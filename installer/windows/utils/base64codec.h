#ifndef BASE64CODEC_H
#define BASE64CODEC_H

#include <string>

namespace Base64Codec
{
std::string encode(const std::string &text);
std::string decode(const std::string &code);
}

#endif // BASE64CODEC_H
