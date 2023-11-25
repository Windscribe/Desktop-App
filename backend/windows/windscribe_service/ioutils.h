#pragma once

class IOUtils
{
public:
    static bool readAll(HANDLE hPipe, char *buf, DWORD len);
    static bool writeAll(HANDLE hPipe, const char *buf, DWORD len);
};
