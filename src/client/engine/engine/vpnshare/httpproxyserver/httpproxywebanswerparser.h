#pragma once

#include <QByteArray>
#include "httpproxyheader.h"  // for shared is_char/is_ctl/etc.
#include "httpproxywebanswer.h"

namespace HttpProxyServer {

class HttpProxyWebAnswerParser
{
public:
    enum class Result { Complete, Incomplete, Malformed, TooLarge };

    // maxBytes bounds what is accumulated before parse() gives up with TooLarge; the owner decides the limit.
    explicit HttpProxyWebAnswerParser(int maxBytes);

    Result parse(const QByteArray &arr, quint32 &outParsed);

    HttpProxyWebAnswer &getAnswer() { return answer_; }

private:
    HttpProxyWebAnswer answer_;
    const int maxBytes_;
    int consumedBytes_ = 0;

    Result consume(char input);

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
