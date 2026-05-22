#pragma once

#include <string>

namespace IO {

// Writes the entire buffer or returns false on error. write(2) can return fewer bytes than
// requested even on regular files (signals, etc.), so we loop until done.
bool writeAll(int fd, const std::string &content);

// Thread-safe replacement for ::strerror(). The libc strerror() returns a pointer into a
// process-wide static buffer that other threads can clobber.
std::string strerror(int err);

} // namespace IO
