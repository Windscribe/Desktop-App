#pragma once

#include <string>

#if defined(WSSECURE_LIBRARY)
#define WSSECURE_EXPORT __declspec(dllexport)
#else
#define WSSECURE_EXPORT __declspec(dllimport)
#endif

WSSECURE_EXPORT bool verifyProcessMitigations(std::wstring &errorMsg);
