#pragma once

// Opens the privileged connection to the helper socket at process startup, while the GUI still
// holds the 'windscribe' group (from its setgid bit), so the process can then permanently drop the
// group -- which libdbus requires before the session bus (tray, notifications) will work. The
// connected fd is handed to HelperBackend_linux, which adopts it instead of connecting itself.
namespace HelperConnector {

// Spawns a background thread that connects to the helper socket (with a bounded retry). Call once,
// early in main(), BEFORE dropping the 'windscribe' group so the thread inherits it.
void startConnect();

// If the startup connect has finished, transfers the connected fd to outFd (or -1 on failure) and
// returns true; returns false while the connect is still in progress. Ownership of the fd passes to
// the caller, which must close it -- a subsequent call yields -1, never the same fd twice.
bool tryGetConnectedFd(int &outFd);

// Gives up on a connect whose fd was never taken: closes the fd if already produced, or marks it to
// be closed when the connect thread later completes. Call when abandoning before tryGetConnectedFd()
// has handed the fd over, so the descriptor is not leaked.
void abandon();

}  // namespace HelperConnector
