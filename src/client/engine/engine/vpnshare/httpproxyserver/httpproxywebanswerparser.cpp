#include "httpproxywebanswerparser.h"
#include "httpproxyheader.h"  // for HttpProxyHeader

namespace HttpProxyServer {

HttpProxyWebAnswerParser::HttpProxyWebAnswerParser(int maxBytes) : maxBytes_(maxBytes), state_(method_start)
{
}

HttpProxyWebAnswerParser::Result HttpProxyWebAnswerParser::parse(const QByteArray &arr, quint32 &outParsed)
{
    const char *data = arr.data();
    for (int i = 0; i < arr.size(); ++i) {
        if (consumedBytes_ >= maxBytes_) {
            outParsed = i;
            return Result::TooLarge;
        }
        ++consumedBytes_;
        const Result res = consume(data[i]);
        if (res != Result::Incomplete) {
            outParsed = i + 1;
            return res;
        }
    }

    outParsed = arr.size();
    return Result::Incomplete;
}

HttpProxyWebAnswerParser::Result HttpProxyWebAnswerParser::consume(char input)
{
    switch (state_)
    {
        case method_start:
            if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return Result::Malformed;
            }
            else
            {
                state_ = method;
                answer_.answer.push_back(input);
                return Result::Incomplete;
            }
        case method:
            if (input == '\r')
            {
                state_ = expecting_newline_1;
                return Result::Incomplete;
            }
            else
            {
                answer_.answer.push_back(input);
                return Result::Incomplete;
            }
        case expecting_newline_1:
            if (input == '\n')
            {
                state_ = header_line_start;
                return Result::Incomplete;
            }
            else
            {
                return Result::Malformed;
            }
        case header_line_start:
            if (input == '\r')
            {
                state_ = expecting_newline_3;
                return Result::Incomplete;
            }
            else if (!answer_.headers.isEmpty() && (input == ' ' || input == '\t'))
            {
                state_ = header_lws;
                return Result::Incomplete;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return Result::Malformed;
            }
            else
            {
                answer_.headers.push_back(HttpProxyHeader());
                answer_.headers.back().name.push_back(input);
                state_ = header_name;
                return Result::Incomplete;
            }
        case header_lws:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return Result::Incomplete;
            }
            else if (input == ' ' || input == '\t')
            {
                return Result::Incomplete;
            }
            else if (is_ctl(input))
            {
                return Result::Malformed;
            }
            else
            {
                state_ = header_value;
                answer_.headers.back().value.push_back(input);
                return Result::Incomplete;
            }
        case header_name:
            if (input == ':')
            {
                state_ = space_before_header_value;
                return Result::Incomplete;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return Result::Malformed;
            }
            else
            {
                answer_.headers.back().name.push_back(input);
                return Result::Incomplete;
            }
        case space_before_header_value:
            if (input == ' ')
            {
                state_ = header_value;
                return Result::Incomplete;
            }
            else
            {
                return Result::Malformed;
            }
        case header_value:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return Result::Incomplete;
            }
            else if (is_ctl(input))
            {
                return Result::Malformed;
            }
            else
            {
                answer_.headers.back().value.push_back(input);
                return Result::Incomplete;
            }
        case expecting_newline_2:
            if (input == '\n')
            {
                state_ = header_line_start;
                return Result::Incomplete;
            }
            else
            {
                return Result::Malformed;
            }
        case expecting_newline_3:
            if (input == '\n')
            {
                return Result::Complete;
            }
            else
            {
                return Result::Malformed;
            }
        default:
            return Result::Malformed;
    }
}

} // namespace HttpProxyServer
