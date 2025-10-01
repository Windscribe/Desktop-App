#pragma once

#include <string>
#include <QVector>
#include <QObject>
#include "httpproxyheader.h"

namespace HttpProxyServer {

// Answer received from a webserver.
struct HttpProxyWebAnswer
{
  std::string answer;
  QVector<HttpProxyHeader> headers;

  long getContentLength();
  std::string processServerHeaders(unsigned int major, unsigned int minor);

private:
  bool shouldSkipHeader(const std::string &headerName);

};

} // namespace HttpProxyServer
