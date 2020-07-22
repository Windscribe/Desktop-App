#include "httprequestparser.h"
#include <boost/algorithm/string.hpp>

HttpRequestParser::HttpRequestParser() : state_(method_start)
{

}

void HttpRequestParser::reset()
{
    state_ = method_start;
}

boost::logic::tribool HttpRequestParser::consume(HttpRequest &req, char input)
{
    switch (state_)
    {
        case method_start:
            if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                state_ = method;
                req.method.push_back(input);
                return boost::indeterminate;
            }
        case method:
            if (input == ' ')
            {
                state_ = uri;
                return boost::indeterminate;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                req.method.push_back(input);
                return boost::indeterminate;
            }
        case uri:
            if (input == ' ')
            {
                state_ = http_version_h;
                return boost::indeterminate;
            }
            else if (is_ctl(input))
            {
                return false;
            }
            else
            {
                req.uri.push_back(input);
                return boost::indeterminate;
            }
        case http_version_h:
            if (input == 'H')
            {
                state_ = http_version_t_1;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_t_1:
            if (input == 'T')
            {
                state_ = http_version_t_2;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_t_2:
            if (input == 'T')
            {
                state_ = http_version_p;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_p:
            if (input == 'P')
            {
                state_ = http_version_slash;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_slash:
            if (input == '/')
            {
                req.http_version_major = 0;
                req.http_version_minor = 0;
                state_ = http_version_major_start;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_major_start:
            if (is_digit(input))
            {
                req.http_version_major = req.http_version_major * 10 + input - '0';
                state_ = http_version_major;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_major:
            if (input == '.')
            {
                state_ = http_version_minor_start;
                return boost::indeterminate;
            }
            else if (is_digit(input))
            {
                req.http_version_major = req.http_version_major * 10 + input - '0';
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_minor_start:
            if (is_digit(input))
            {
                req.http_version_minor = req.http_version_minor * 10 + input - '0';
                state_ = http_version_minor;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_minor:
            if (input == '\r')
            {
                state_ = expecting_newline_1;
                return boost::indeterminate;
            }
            else if (is_digit(input))
            {
                req.http_version_minor = req.http_version_minor * 10 + input - '0';
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case expecting_newline_1:
            if (input == '\n')
            {
                state_ = header_line_start;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case header_line_start:
            if (input == '\r')
            {
                state_ = expecting_newline_3;
                return boost::indeterminate;
            }
            else if (!req.headers.empty() && (input == ' ' || input == '\t'))
            {
                state_ = header_lws;
                return boost::indeterminate;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                req.headers.push_back(HttpHeader());
                req.headers.back().name.push_back(input);
                state_ = header_name;
                return boost::indeterminate;
            }
        case header_lws:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return boost::indeterminate;
            }
            else if (input == ' ' || input == '\t')
            {
                return boost::indeterminate;
            }
            else if (is_ctl(input))
            {
                return false;
            }
            else
            {
                state_ = header_value;
                req.headers.back().value.push_back(input);
                return boost::indeterminate;
            }
        case header_name:
            if (input == ':')
            {
                state_ = space_before_header_value;
                return boost::indeterminate;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                req.headers.back().name.push_back(input);
                return boost::indeterminate;
            }
        case space_before_header_value:
            if (input == ' ')
            {
                state_ = header_value;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case header_value:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return boost::indeterminate;
            }
            else if (is_ctl(input))
            {
                return false;
            }
            else
            {
                req.headers.back().value.push_back(input);
                return boost::indeterminate;
            }
        case expecting_newline_2:
            if (input == '\n')
            {
                state_ = header_line_start;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
		case expecting_newline_3:
		{
			if (input == '\n')
			{
				contentLength_ = contentLength(req);
				if (contentLength_ == 0)
				{
					return false;
				}
				else if (contentLength_ > MAX_CONTENT_LENGTH)
				{
					return false;
				}
				else
				{
					state_ = read_content;
					return boost::indeterminate;
				}
			}
			else
			{
				return false;
			}
		}
		case read_content:
		{
			req.content.push_back(input);
			contentLength_--;
			if (contentLength_ == 0)
			{
				return true;
			}
			else
			{
				return boost::indeterminate;
			}
		}
        default:
            return false;
    }
}

int HttpRequestParser::contentLength(HttpRequest& req)
{
	for (std::vector<HttpHeader>::iterator it = req.headers.begin(); it != req.headers.end(); ++it)
	{
		if (boost::iequals(it->name, "Content-Length"))
		{
			return std::stoi(it->value);
		}
	}
	return 0;
}

bool HttpRequestParser::is_char(int c)
{
    return c >= 0 && c <= 127;
}

bool HttpRequestParser::is_ctl(int c)
{
    return (c >= 0 && c <= 31) || (c == 127);
}

bool HttpRequestParser::is_tspecial(int c)
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

bool HttpRequestParser::is_digit(int c)
{
    return c >= '0' && c <= '9';
}