#ifndef HTTPREQUESTPARSER_H
#define HTTPREQUESTPARSER_H

#include "httprequest.h"
#include <boost/tuple/tuple.hpp>
#include <boost/logic/tribool.hpp>

class HttpRequestParser
{
public:
    HttpRequestParser();

    void reset();

    // Parse some data. The tribool return value is true when a complete request
    // has been parsed, false if the data is invalid, indeterminate when more
    // data is required. The InputIterator return value indicates how much of the
    // input has been consumed.
    template <typename InputIterator>
    boost::tuple<boost::tribool, InputIterator> parse(HttpRequest& req, InputIterator begin, InputIterator end)
    {
        while (begin != end)
        {
            boost::tribool result = consume(req, *begin++);
            if (result || !result)
                return boost::make_tuple(result, begin);
        }
        boost::tribool result = boost::indeterminate;
        return boost::make_tuple(result, begin);
    }

private:
    boost::tribool consume(HttpRequest& req, char input);
	int contentLength(HttpRequest& req);

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
        expecting_newline_3,
		read_content
    } state_;

	static constexpr int MAX_CONTENT_LENGTH = 200;
	int contentLength_;
};

#endif // HTTPREQUESTPARSER_H
