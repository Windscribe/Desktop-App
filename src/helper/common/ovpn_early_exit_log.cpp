#include "ovpn_early_exit_log.h"

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <wchar.h>
#endif

namespace OvpnEarlyExitLog {

namespace {

constexpr size_t kMaxDetailLength = 500;

// OpenVPN prefixes each --log line with a timestamp such as "2026-10-08 12:00:00 ".
// A line that does not start with four digits and a dash is returned unchanged.
std::string stripTimestamp(const std::string &line)
{
    if (line.size() < 20 || line[4] != '-') {
        return line;
    }
    for (int i = 0; i < 4; ++i) {
        if (line[i] < '0' || line[i] > '9') {
            return line;
        }
    }
    // "YYYY-MM-DD HH:MM:SS" is 19 characters; OpenVPN may append " us=NNNNNN" before the message.
    const size_t dateSpace = line.find(' ');
    if (dateSpace == std::string::npos || dateSpace + 9 >= line.size()) {
        return line;
    }
    size_t message = dateSpace + 9;
    if (line.compare(message, 4, " us=") == 0) {
        const size_t afterUs = line.find(' ', message + 4);
        if (afterUs == std::string::npos) {
            return line;
        }
        message = afterUs + 1;
    } else if (line[message] == ' ') {
        message += 1;
    } else {
        return line;
    }
    return line.substr(message);
}

bool isActionable(const std::string &line)
{
    return line.find("Windscribe:") != std::string::npos
        || line.find("Options error:") != std::string::npos;
}

} // namespace

namespace {

std::string readOpenFile(FILE *file)
{
    if (!file) {
        return std::string();
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return std::string();
    }
    const long size = std::ftell(file);
    if (size < 0) {
        std::fclose(file);
        return std::string();
    }
    constexpr long kTailBytes = 4096;
    const long offset = size > kTailBytes ? size - kTailBytes : 0;
    if (std::fseek(file, offset, SEEK_SET) != 0) {
        std::fclose(file);
        return std::string();
    }
    std::string text(static_cast<size_t>(size - offset), '\0');
    const size_t read = std::fread(text.data(), 1, text.size(), file);
    std::fclose(file);
    text.resize(read);
    // A tail that starts mid-line would keep a fragment. Drop it.
    if (offset > 0) {
        const size_t newline = text.find('\n');
        if (newline == std::string::npos) {
            return std::string();
        }
        text.erase(0, newline + 1);
    }
    return extract(text);
}

} // namespace

#ifdef _WIN32
std::string readFile(const std::wstring &path)
{
    FILE *file = _wfopen(path.c_str(), L"rb");
    return readOpenFile(file);
}
#else
std::string readFile(const std::string &path)
{
    FILE *file = std::fopen(path.c_str(), "rb");
    return readOpenFile(file);
}
#endif

std::string extract(const std::string &logText)
{
    std::string detail;
    size_t begin = 0;
    while (begin <= logText.size()) {
        const size_t end = logText.find('\n', begin);
        std::string line = logText.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        line = stripTimestamp(line);
        if (isActionable(line)) {
            if (!detail.empty()) {
                detail += '\n';
            }
            detail += line;
            if (detail.size() >= kMaxDetailLength) {
                detail.resize(kMaxDetailLength);
                break;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return detail;
}

} // namespace OvpnEarlyExitLog
