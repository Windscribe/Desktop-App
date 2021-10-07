#include "all_headers.h"
#include "ioutils.h"

bool IOUtils::readAll(HANDLE hPipe, char *buf, DWORD len)
{
    char *ptr = buf;
    DWORD dwRead = 0;
	DWORD lenCopy = len;
	while (lenCopy > 0)
    {
		if (::ReadFile(hPipe, ptr, lenCopy, &dwRead, 0))
        {
            ptr += dwRead;
			lenCopy -= dwRead;
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool IOUtils::writeAll(HANDLE hPipe, const char *buf, DWORD len)
{
	const char *ptr = buf;
    DWORD dwWrite = 0;
    while (len > 0)
    {
        if (::WriteFile(hPipe, ptr, len, &dwWrite, 0))
        {
            ptr += dwWrite;
            len -= dwWrite;
        }
        else
        {
            return false;
        }
    }
    return true;
}
