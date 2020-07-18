#include "httpproxyreply.h"
#include "Utils/boost_includes.h"

namespace HttpProxyServer {

namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

QByteArray to_buffer(HttpProxyReply::status_type status)
{
    switch (status)
    {
    case HttpProxyReply::ok:
        return QByteArray(ok.c_str(), ok.length());
    case HttpProxyReply::created:
        return QByteArray(created.c_str(), created.length());
    case HttpProxyReply::accepted:
        return QByteArray(accepted.c_str(), accepted.length());
    case HttpProxyReply::no_content:
        return QByteArray(no_content.c_str(), no_content.length());
    case HttpProxyReply::multiple_choices:
        return QByteArray(multiple_choices.c_str(), multiple_choices.length());
    case HttpProxyReply::moved_permanently:
        return QByteArray(moved_permanently.c_str(), moved_permanently.length());
    case HttpProxyReply::moved_temporarily:
        return QByteArray(moved_temporarily.c_str(), moved_temporarily.length());
    case HttpProxyReply::not_modified:
        return QByteArray(not_modified.c_str(), not_modified.length());
    case HttpProxyReply::bad_request:
        return QByteArray(bad_request.c_str(), bad_request.length());
    case HttpProxyReply::unauthorized:
        return QByteArray(unauthorized.c_str(), unauthorized.length());
    case HttpProxyReply::forbidden:
        return QByteArray(forbidden.c_str(), forbidden.length());
    case HttpProxyReply::not_found:
        return QByteArray(not_found.c_str(), not_found.length());
    case HttpProxyReply::internal_server_error:
        return QByteArray(internal_server_error.c_str(), internal_server_error.length());
    case HttpProxyReply::not_implemented:
        return QByteArray(not_implemented.c_str(), not_implemented.length());
    case HttpProxyReply::bad_gateway:
        return QByteArray(bad_gateway.c_str(), bad_gateway.length());
    case HttpProxyReply::service_unavailable:
        return QByteArray(service_unavailable.c_str(), service_unavailable.length());
    default:
        return QByteArray(internal_server_error.c_str(), internal_server_error.length());
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

QByteArray HttpProxyReply::toBuffer()
{
    QByteArray arr;
    arr.append(status_strings::to_buffer(status));
    for (std::size_t i = 0; i < headers.size(); ++i)
    {
        HttpProxyHeader &h = headers[i];
        arr.append(h.name.c_str(), h.name.length());
        arr.append(misc_strings::name_value_separator, sizeof(misc_strings::name_value_separator));
        arr.append(h.value.c_str(), h.value.length());
        arr.append(misc_strings::crlf, sizeof(misc_strings::crlf));
    }
    arr.append(misc_strings::crlf, sizeof(misc_strings::crlf));
    arr.append(content.c_str(), content.length());
    /*buffers.push_back(boost::asio::buffer(misc_strings::crlf));
    buffers.push_back(boost::asio::buffer(content));
    return buffers;*/
    return arr;
}


namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(HttpProxyReply::status_type status)
{
  switch (status)
  {
  case HttpProxyReply::ok:
    return ok;
  case HttpProxyReply::created:
    return created;
  case HttpProxyReply::accepted:
    return accepted;
  case HttpProxyReply::no_content:
    return no_content;
  case HttpProxyReply::multiple_choices:
    return multiple_choices;
  case HttpProxyReply::moved_permanently:
    return moved_permanently;
  case HttpProxyReply::moved_temporarily:
    return moved_temporarily;
  case HttpProxyReply::not_modified:
    return not_modified;
  case HttpProxyReply::bad_request:
    return bad_request;
  case HttpProxyReply::unauthorized:
    return unauthorized;
  case HttpProxyReply::forbidden:
    return forbidden;
  case HttpProxyReply::not_found:
    return not_found;
  case HttpProxyReply::internal_server_error:
    return internal_server_error;
  case HttpProxyReply::not_implemented:
    return not_implemented;
  case HttpProxyReply::bad_gateway:
    return bad_gateway;
  case HttpProxyReply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

HttpProxyReply HttpProxyReply::stock_reply(HttpProxyReply::status_type status)
{
    HttpProxyReply rep;
    rep.status = status;
    rep.content = stock_replies::to_string(status);
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "text/html";
    return rep;
}


} // namespace HttpProxyServer
