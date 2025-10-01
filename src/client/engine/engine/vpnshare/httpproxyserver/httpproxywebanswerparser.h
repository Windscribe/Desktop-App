#pragma once

#include "utils/boost_includes.h"
#include "httpproxywebanswer.h"
#include "httpproxyrequestparser.h"

namespace HttpProxyServer {

class HttpProxyWebAnswerParser
{
public:
    HttpProxyWebAnswerParser();

    TRI_BOOL parse(const QByteArray &arr, quint32 &outParsed);

    HttpProxyWebAnswer &getAnswer() { return answer_; }

private:
    HttpProxyWebAnswer answer_;

    TRI_BOOL consume(char input);

    static bool is_char(int c);
    static bool is_ctl(int c);
    static bool is_tspecial(int c);
    static bool is_digit(int c);

    enum state
    {
        method_start,
        method,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } state_;
};

} // namespace HttpProxyServer
