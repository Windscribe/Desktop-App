#include "io_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
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

bool writeFile(const std::string &path, const std::string &content, int mode)
{
    unlink(path.c_str());
    int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd < 0) {
        return false;
    }
    if (!writeAll(fd, content)) {
        close(fd);
        unlink(path.c_str());
        return false;
    }
    close(fd);
    return true;
}

bool ensureRootOwnedFile(const std::string &path, int mode)
{
    // O_CLOEXEC: the helper forks children concurrently, and the fd must not leak into them.
    int fd = open(path.c_str(), O_CREAT | O_RDWR | O_NOFOLLOW | O_CLOEXEC, mode);
    if (fd < 0) {
        return false;
    }
    struct stat st;
    // Ownership before mode, so a pre-existing file with a wrong owner never gets the new mode.
    const bool ok = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && fchown(fd, 0, 0) == 0 && fchmod(fd, mode) == 0;
    close(fd);
    return ok;
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
