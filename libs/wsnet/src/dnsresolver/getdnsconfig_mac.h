#pragma once
#include <string>

namespace wsnet {

// returns a list of DNS servers installed in MacOS in CSV format using the System Configuration Framework
std::string getDnsConfig_mac();

} // namespace wsnet
