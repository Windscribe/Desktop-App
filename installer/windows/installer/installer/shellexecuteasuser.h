#pragma once

#include <Windows.h>

namespace ShellExec
{

void executeFromExplorer(const wchar_t *pszFile, const wchar_t *pszParameters = L"", const wchar_t *pszDirectory = L"",
                         const wchar_t *pszOperation = L"", int nShowCmd = SW_SHOWNORMAL);

};