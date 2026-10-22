#pragma once

#include <string>

namespace OvpnEarlyExitLog {

// Pull the lines a user can act on out of an OpenVPN --log file read after the daemon
// exited before binding its management socket. Keeps lines containing "Windscribe:" or
// "Options error:", drops OpenVPN's timestamp prefix, and caps the result at 500 characters.
// Other log noise is omitted. An empty result means the file had nothing to show.
std::string extract(const std::string &logText);

// Reads at most the last 4 KiB of path and returns extract() of that text. An unreadable or
// empty file returns an empty string. The Windows path is wide so a non-ASCII Program Files
// directory is opened as written.
#ifdef _WIN32
std::string readFile(const std::wstring &path);
#else
std::string readFile(const std::string &path);
#endif

} // namespace OvpnEarlyExitLog
