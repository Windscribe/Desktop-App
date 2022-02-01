#include "httpproxywebanswerparser.h"

namespace HttpProxyServer {

HttpProxyWebAnswerParser::HttpProxyWebAnswerParser() : state_(method_start)
{

}

TRI_BOOL HttpProxyWebAnswerParser::parse(const QByteArray &arr, quint32 &outParsed)
{
    const char *data = arr.data();
    for (int i = 0; i < arr.size(); ++i)
    {
        TRI_BOOL res = consume(data[i]);
        if (res == TRI_TRUE || res == TRI_FALSE)
        {
            outParsed = i + 1;
            return res;
        }
    }

    outParsed = arr.size();
    return TRI_INDETERMINATE;
}

TRI_BOOL HttpProxyWebAnswerParser::consume(char input)
{
    switch (state_)
    {
        case method_start:
            if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return TRI_FALSE;
            }
            else
            {
                state_ = method;
                answer_.answer.push_back(input);
                return TRI_INDETERMINATE;
            }
        case method:
            if (input == '\r')
            {
                state_ = expecting_newline_1;
                return TRI_INDETERMINATE;
            }
            else
            {
                answer_.answer.push_back(input);
                return TRI_INDETERMINATE;
            }
        case expecting_newline_1:
            if (input == '\n')
            {
                state_ = header_line_start;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case header_line_start:
            if (input == '\r')
            {
                state_ = expecting_newline_3;
                return TRI_INDETERMINATE;
            }
            else if (!answer_.headers.isEmpty() && (input == ' ' || input == '\t'))
            {
                state_ = header_lws;
                return TRI_INDETERMINATE;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return TRI_FALSE;
            }
            else
            {
                answer_.headers.push_back(HttpProxyHeader());
                answer_.headers.back().name.push_back(input);
                state_ = header_name;
                return TRI_INDETERMINATE;
            }
        case header_lws:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return TRI_INDETERMINATE;
            }
            else if (input == ' ' || input == '\t')
            {
                return TRI_INDETERMINATE;
            }
            else if (is_ctl(input))
            {
                return TRI_FALSE;
            }
            else
            {
                state_ = header_value;
                answer_.headers.back().value.push_back(input);
                return TRI_INDETERMINATE;
            }
        case header_name:
            if (input == ':')
            {
                state_ = space_before_header_value;
                return TRI_INDETERMINATE;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return TRI_FALSE;
            }
            else
            {
                answer_.headers.back().name.push_back(input);
                return TRI_INDETERMINATE;
            }
        case space_before_header_value:
            if (input == ' ')
            {
                state_ = header_value;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case header_value:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return TRI_INDETERMINATE;
            }
            else if (is_ctl(input))
            {
                return TRI_FALSE;
            }
            else
            {
                answer_.headers.back().value.push_back(input);
                return TRI_INDETERMINATE;
            }
        case expecting_newline_2:
            if (input == '\n')
            {
                state_ = header_line_start;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case expecting_newline_3:
            if (input == '\n')
            {
                return TRI_TRUE;
            }
            else
            {
                return TRI_FALSE;
            }
        default:
            return TRI_FALSE;
    }
}

bool HttpProxyWebAnswerParser::is_char(int c)
{
    return c >= 0 && c <= 127;
}

bool HttpProxyWebAnswerParser::is_ctl(int c)
{
    return (c >= 0 && c <= 31) || (c == 127);
}

bool HttpProxyWebAnswerParser::is_tspecial(int c)
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

bool HttpProxyWebAnswerParser::is_digit(int c)
{
    return c >= '0' && c <= '9';
}

} // namespace HttpProxyServer
