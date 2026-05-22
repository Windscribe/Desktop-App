#include "io_posix.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

namespace IO {

bool writeAll(int fd, const std::string &content)
{
    const char *p = content.c_str();
    size_t remaining = content.length();
    while (remaining > 0) {
        ssize_t n = ::write(fd, p, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        p += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

namespace {
// Two overloads absorb the XSI-vs-GNU strerror_r() return-type split:
//   XSI (macOS, POSIX):  int strerror_r(int, char *, size_t)   — writes buf, returns 0/errno
//   GNU (glibc default): char *strerror_r(int, char *, size_t) — may return a pointer to a
//                                                                static string instead of buf
// The compiler picks the matching overload at compile time per platform; no #ifdef needed.
const char *fromStrerrorR(int /*ret*/, const char *buf) { return buf; }
const char *fromStrerrorR(const char *ret, const char * /*buf*/) { return ret; }
} // namespace

std::string strerror(int err)
{
    char buf[256];
    return fromStrerrorR(::strerror_r(err, buf, sizeof(buf)), buf);
}

} // namespace IO
