#pragma once

#include <string>

namespace HttpProxyServer {

// Character classification helpers used by both HTTP request and web-answer parsers.
// These match the rules from RFC 7230 / original HTTP/1.1 spec for header parsing.
inline bool is_char(int c) { return c >= 0 && c <= 127; }
inline bool is_ctl(int c) { return (c >= 0 && c <= 31) || (c == 127); }
inline bool is_tspecial(int c)
{
    switch (c)
    {
        case '(': case ')': case '<': case '>': case '@':
        case ',': case ';': case ':': case '\\': case '"':
        case '/': case '[': case ']': case '?': case '=':
        case '{': case '}': case ' ': case '\t':
            return true;
        default:
            return false;
    }
}
inline bool is_digit(int c) { return c >= '0' && c <= '9'; }

struct HttpProxyHeader
{
  std::string name;
  std::string value;
};

} // namespace HttpProxyServer
