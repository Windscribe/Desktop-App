#ifndef HTTPPROXYHEADER_H
#define HTTPPROXYHEADER_H

#include <string>

namespace HttpProxyServer {

struct HttpProxyHeader
{
  std::string name;
  std::string value;
};

} // namespace HttpProxyServer

#endif // HTTPPROXYHEADER_H
