#ifndef UTILS_H
#define UTILS_H

#include <string>

#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }

namespace Utils
{
    std::wstring defaultInstallPath();
    bool in32BitProgramFilesFolder(const std::wstring path);
}

#endif // UTILS_H
