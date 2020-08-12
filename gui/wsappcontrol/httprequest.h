#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <vector>
#include "httpheader.h"

// A request received from a client.
struct HttpRequest
{
  HttpRequest() : http_version_major(0), http_version_minor(0) {}

  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<HttpHeader> headers;
  std::string content;
};

#endif // HTTPREQUEST_H
