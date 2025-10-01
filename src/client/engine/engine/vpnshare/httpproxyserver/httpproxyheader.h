#pragma once

#include <string>

namespace HttpProxyServer {

struct HttpProxyHeader
{
  std::string name;
  std::string value;
};

} // namespace HttpProxyServer
