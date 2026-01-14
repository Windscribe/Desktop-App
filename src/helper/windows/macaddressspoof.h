#pragma once

#include <string>

namespace MacAddressSpoof
{

void set(const std::wstring &interfaceName, const std::wstring &value);
void remove(const std::wstring &interfaceName);

}
