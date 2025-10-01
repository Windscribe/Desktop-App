#pragma once

#include <Windows.h>

namespace IOUtils {

bool readAll(HANDLE hPipe, HANDLE hIOEvent, char *buf, DWORD len);
bool writeAll(HANDLE hPipe, HANDLE hIOEvent, const char *buf, DWORD len);

}
