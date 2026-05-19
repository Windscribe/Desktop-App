#pragma once

#include <string>

// Shell-safe quoting for values interpolated into command strings.
// Wraps the value in single quotes and escapes any embedded single quotes
// using the standard '\'' pattern (end quote, escaped literal quote, reopen quote).
// Inside single quotes, the shell performs no expansion — $(), backticks,
// and all other metacharacters are treated as literal text.

namespace ShellQuote {

inline std::string quote(const std::string &value)
{
    std::string result = "'";
    for (char c : value) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += "'";
    return result;
}

} // namespace ShellQuote
