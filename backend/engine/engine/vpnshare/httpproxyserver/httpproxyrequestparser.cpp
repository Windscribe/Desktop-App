#include "httpproxyrequestparser.h"

namespace HttpProxyServer {


HttpProxyRequestParser::HttpProxyRequestParser() : state_(method_start)
{

}

TRI_BOOL HttpProxyRequestParser::parse(const QByteArray &arr, quint32 &outParsed)
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

TRI_BOOL HttpProxyRequestParser::consume(char input)
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
                request_.method.push_back(input);
                return TRI_INDETERMINATE;
            }
        case method:
            if (input == ' ')
            {
                state_ = uri;
                return TRI_INDETERMINATE;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return TRI_FALSE;
            }
            else
            {
                request_.method.push_back(input);
                return TRI_INDETERMINATE;
            }
        case uri:
            if (input == ' ')
            {
                state_ = http_version_h;
                return TRI_INDETERMINATE;
            }
            else if (is_ctl(input))
            {
                return TRI_FALSE;
            }
            else
            {
                request_.uri.push_back(input);
                return TRI_INDETERMINATE;
            }
        case http_version_h:
            if (input == 'H')
            {
                state_ = http_version_t_1;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_t_1:
            if (input == 'T')
            {
                state_ = http_version_t_2;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_t_2:
            if (input == 'T')
            {
                state_ = http_version_p;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_p:
            if (input == 'P')
            {
                state_ = http_version_slash;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_slash:
            if (input == '/')
            {
                request_.http_version_major = 0;
                request_.http_version_minor = 0;
                state_ = http_version_major_start;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_major_start:
            if (is_digit(input))
            {
                request_.http_version_major = request_.http_version_major * 10 + input - '0';
                state_ = http_version_major;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_major:
            if (input == '.')
            {
                state_ = http_version_minor_start;
                return TRI_INDETERMINATE;
            }
            else if (is_digit(input))
            {
                request_.http_version_major = request_.http_version_major * 10 + input - '0';
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_minor_start:
            if (is_digit(input))
            {
                request_.http_version_minor = request_.http_version_minor * 10 + input - '0';
                state_ = http_version_minor;
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
            }
        case http_version_minor:
            if (input == '\r')
            {
                state_ = expecting_newline_1;
                return TRI_INDETERMINATE;
            }
            else if (is_digit(input))
            {
                request_.http_version_minor = request_.http_version_minor * 10 + input - '0';
                return TRI_INDETERMINATE;
            }
            else
            {
                return TRI_FALSE;
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
            else if (!request_.headers.isEmpty() && (input == ' ' || input == '\t'))
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
                request_.headers.push_back(HttpProxyHeader());
                request_.headers.back().name.push_back(input);
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
                request_.headers.back().value.push_back(input);
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
                request_.headers.back().name.push_back(input);
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
                request_.headers.back().value.push_back(input);
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
            else {
                return TRI_FALSE;
            }
        default:
            return TRI_FALSE;
        }
}

bool HttpProxyRequestParser::is_char(int c)
{
    return c >= 0 && c <= 127;
}

bool HttpProxyRequestParser::is_ctl(int c)
{
    return (c >= 0 && c <= 31) || (c == 127);
}

bool HttpProxyRequestParser::is_tspecial(int c)
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

bool HttpProxyRequestParser::is_digit(int c)
{
    return c >= '0' && c <= '9';
}

} // namespace HttpProxyServer
