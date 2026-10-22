#pragma once

#include <string>

namespace IO {

// Writes the entire buffer or returns false on error. write(2) can return fewer bytes than
// requested even on regular files (signals, etc.), so we loop until done.
bool writeAll(int fd, const std::string &content);

// Writes content to path with the given mode, replacing any existing file. open()'s mode is
// ignored on a pre-existing file, so the path is unlinked first to force the intended perms.
// On any failure the (possibly partial) file is removed and false is returned.
bool writeFile(const std::string &path, const std::string &content, int mode);

// Reads the whole file into content. Returns false (content untouched) when it cannot be opened
// or read.
bool readFile(const std::string &path, std::string &content);

// Creates path if missing, then makes it a root-owned regular file with the given mode. An existing
// file is kept rather than recreated, so a lock that a running process holds on it survives.
bool ensureRootOwnedFile(const std::string &path, int mode);

// unlink(2); false when the file existed but could not be removed (a missing file is success).
bool removeFile(const std::string &path);

// Thread-safe replacement for ::strerror(). The libc strerror() returns a pointer into a
// process-wide static buffer that other threads can clobber.
std::string strerror(int err);

} // namespace IO
