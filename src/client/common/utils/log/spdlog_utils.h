#pragma once

#include <fstream>
#include <spdlog/pattern_formatter.h>

namespace log_utils {

// The implementation is taken from  https://github.com/nlohmann/json/blob/ec7a1d834773f9fee90d8ae908a0c9933c5646fc/src/json.hpp#L4604-L4697
static std::size_t extra_space(const spdlog::string_view_t& s) noexcept
{
    std::size_t result = 0;

    for (const auto& c : s)
    {
        switch (c)
        {
        case '"':
        case '\\':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        {
            // from c (1 byte) to \x (2 bytes)
            result += 1;
            break;
        }

        default:
        {
            if (c >= 0x00 && c <= 0x1f)
            {
                // from c (1 byte) to \uxxxx (6 bytes)
                result += 5;
            }
            break;
        }
        }
    }

    return result;
}

// The implementation is taken from  https://github.com/nlohmann/json/blob/ec7a1d834773f9fee90d8ae908a0c9933c5646fc/src/json.hpp#L4604-L4697
static std::string escape_string(const spdlog::string_view_t& s) noexcept
{
    const auto space = extra_space(s);
    if (space == 0)
    {
        return std::string(s.data(), s.size());
    }

    // create a result string of necessary size
    std::string result(s.size() + space, '\\');
    std::size_t pos = 0;

    for (const auto& c : s)
    {
        switch (c)
        {
        // quotation mark (0x22)
        case '"':
        {
            result[pos + 1] = '"';
            pos += 2;
            break;
        }

            // reverse solidus (0x5c)
        case '\\':
        {
            // nothing to change
            pos += 2;
            break;
        }

            // backspace (0x08)
        case '\b':
        {
            result[pos + 1] = 'b';
            pos += 2;
            break;
        }

            // formfeed (0x0c)
        case '\f':
        {
            result[pos + 1] = 'f';
            pos += 2;
            break;
        }

            // newline (0x0a)
        case '\n':
        {
            result[pos + 1] = 'n';
            pos += 2;
            break;
        }

            // carriage return (0x0d)
        case '\r':
        {
            result[pos + 1] = 'r';
            pos += 2;
            break;
        }

            // horizontal tab (0x09)
        case '\t':
        {
            result[pos + 1] = 't';
            pos += 2;
            break;
        }

        default:
        {
            if (c >= 0x00 && c <= 0x1f)
            {
                // print character c as \uxxxx
                snprintf(&result[pos + 1], 10, "u%04x", int(c));
                pos += 6;
                // overwrite trailing null character
                result[pos] = '\\';
            }
            else
            {
                // all other characters are added as-is
                result[pos++] = c;
            }
            break;
        }
        }
    }

    return result;
}

// For formatting escapes strings for json
class escapeFormatterFlag : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest) override
    {
        std::string escapedStr = escape_string(msg.payload);
        dest.append(escapedStr.data(), escapedStr.data() + escapedStr.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<escapeFormatterFlag>();
    }
};

// For formatting escapes strings for json
class customFormatterFlag : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest) override
    {
        std::string escapedStr = escape_string(msg.payload);
        dest.append(escapedStr.data(), escapedStr.data() + escapedStr.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<escapeFormatterFlag>();
    }
};


// quick check if the file is a new or old log format
// check that the first 3 lines start and end with symbols { and }
template<typename T>
static bool isOldLogFormat(const T &path)
{
    std::ifstream infile(path);
    std::string line;
    size_t num = 0;
    bool isOldFormat = false;
    while (std::getline(infile, line)) {
        if (line.length() >= 2) {
            if (line[0] != '{' || line[line.length() - 1] != '}') {
                isOldFormat = true;
                break;
            }
        }
        if (num >= 2) {
            break;
        }
        num++;
    }
    return isOldFormat;
}

// Create json pattern
static std::unique_ptr<spdlog::pattern_formatter> createJsonFormatter()
{
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<escapeFormatterFlag>('*');
    std::string jsonpattern = {"{\"tm\": \"%Y-%m-%d %H:%M:%S.%e\", \"lvl\": \"%^%l%$\", \"mod\": \"%n\", \"msg\": \"%*\"}"};
    formatter->set_pattern(jsonpattern);
    return formatter;
}

}  // namespace log_utils
