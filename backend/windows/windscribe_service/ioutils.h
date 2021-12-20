#ifndef IOUTILS_H
#define IOUTILS_H

class IOUtils
{
public:
    static bool readAll(HANDLE hPipe, char *buf, DWORD len);
    static bool writeAll(HANDLE hPipe, const char *buf, DWORD len);
};

#endif // IOUTILS_H
