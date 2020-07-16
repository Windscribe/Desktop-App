#include "all_headers.h"
#include "ioutils.h"
#include "simple_xor_crypt.h"
#include "ipc/servicecommunication.h"

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

	std::string s(buf, len);
	std::string d = SimpleXorCrypt::decrypt(s, ENCRYPT_KEY);
	memcpy(buf, d.data(), len);

    return true;
}

bool IOUtils::writeAll(HANDLE hPipe, const char *buf, DWORD len)
{
	std::string s(buf, len);
	std::string d = SimpleXorCrypt::encrypt(s, ENCRYPT_KEY);

	const char *ptr = d.data();
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
