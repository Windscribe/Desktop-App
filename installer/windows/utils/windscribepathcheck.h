#pragma once
#include <string>

namespace PathCheck
{
    bool isNeedAppendSubdirectory(const std::wstring &installPath, const std::wstring &prevInstallPath);
}
