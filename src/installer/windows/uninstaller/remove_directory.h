#pragma once

#include <string>

namespace RemoveDir
{

bool DelTree(const std::wstring Path, const bool IsDir, const bool DeleteFiles,
             const bool DeleteSubdirsAlso, const bool breakOnError);

};
